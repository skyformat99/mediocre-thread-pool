#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread-pool.h"

#define exit_routine (void *)-1L    // 0xffffffffffffffff

#define MIN_WAITING_WORKERS   8
#define MAX_WAITING_WORKERS  500
#define EVALUATION_COUNTER     1000

static void *start_routine(void *args) {
    task_t task = { 0 };
    thread_pool_t *this = args;

    while (1) {

        /* ****************************************************************** */


        atomic_fetch_add_explicit(&this->waiting_workers, 
            1, memory_order_relaxed);

        pthread_mutex_lock(&this->mutex_for_pool);

        if (this->shutdown_pool) {
            pthread_mutex_unlock(&this->mutex_for_pool);
            break;
        }


        /* ****************************************************************** */


        if (this->scale_down) {
            if (this->curr_barred_workers != WORKERS_PER_GROUP) {
                this->curr_barred_workers += 1;
                pthread_barrier_t *const barrier_ptr = 
                    &this->barriers[this->curr_barrier_size];
                pthread_mutex_unlock(&this->mutex_for_pool);
                pthread_barrier_wait(barrier_ptr);

                if (! this->shutdown_barred_group) {
                    continue;
                } else {
                    break;
                }

            } else {
                this->scale_down = false;
                this->curr_barrier_size += 1;
                this->curr_barred_workers = 0;                
                pthread_cond_signal(&this->barring_completed);
                atomic_fetch_sub_explicit(&this->waiting_workers,
                    WORKERS_PER_GROUP, memory_order_relaxed);
            }


        /* ****************************************************************** */


        } else if (this->scale_up) {
            this->scale_up = false;
            this->curr_barrier_size -= 1;
            pthread_barrier_wait(&this->barriers[this->curr_barrier_size]);
            pthread_cond_signal(&this->barring_completed);
        }


        /* ****************************************************************** */


        if (! this->shutdown_barred_group &&
            ++this->eval_cnt == EVALUATION_COUNTER) {
            if (0.7 * RING_QUEUE_CAPACITY < size(&this->task_queue)) {
                int waiting_workers_ = atomic_load_explicit(
                    &this->waiting_workers, memory_order_relaxed);

                this->scale_up = (waiting_workers_ < MIN_WAITING_WORKERS &&
                    0 != this->curr_barrier_size);

                this->scale_down = (waiting_workers_ > MAX_WAITING_WORKERS &&
                    this->group_size - 1 != this->curr_barrier_size);
            }

            this->eval_cnt = 0;
        }


        /* ****************************************************************** */


        pthread_mutex_lock(&this->mutex_for_queue);

        if (is_empty(&this->task_queue)) {
            pthread_cond_wait(&this->task_available, &this->mutex_for_queue);
        }

        if (is_full(&this->task_queue)) {
            pthread_cond_signal(&this->space_available);
        }

         if (-1 == task_queue_pop(&this->task_queue, &task)) {
            fprintf(stderr, "Empty queue exception.\n");
            pthread_mutex_unlock(&this->mutex_for_queue);
            pthread_mutex_unlock(&this->mutex_for_pool);
            pthread_exit(NULL);
        }

        pthread_mutex_unlock(&this->mutex_for_queue);


        /* ****************************************************************** */


        if (task.run == exit_routine) {
            this->shutdown_pool = true;
            pthread_mutex_unlock(&this->mutex_for_pool);
            break;
        }

        pthread_mutex_unlock(&this->mutex_for_pool);

        atomic_fetch_sub_explicit(&this->waiting_workers,
            1, memory_order_relaxed);

        task.run(task.arguments);


        /* ****************************************************************** */

    }

    pthread_exit(NULL);
}

int thread_pool_init(thread_pool_t *this, const int group_size) {
    if (NULL == this) {
        fprintf(stderr, "Null pointer exception.\n");
        return -1;
    }

    if (0 >= group_size) {
        fprintf(stderr, "Invalid size of thread pool.\n");
        return -1;
    }


    // Initialize all integer/boolean attributes to zero/false.
    memset(this, 0, sizeof(thread_pool_t));
    task_queue_init(&this->task_queue);

    if (-1 == pthread_cond_init(&this->task_available, NULL) ||
        -1 == pthread_cond_init(&this->space_available, NULL) ||
        -1 == pthread_cond_init(&this->barring_completed, NULL)) {
        perror("pthread_cond_init");
        goto Error;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);

    if (-1 == pthread_mutex_init(&this->mutex_for_pool, &attr) ||
        -1 == pthread_mutex_init(&this->mutex_for_queue, &attr)) {
        perror("pthread_mutex_init");
        goto Error;
    }

    pthread_mutexattr_destroy(&attr);

    this->group_size = group_size;

    this->barriers = (pthread_barrier_t *)malloc(
        this->group_size * sizeof(pthread_barrier_t));

    this->workers = (pthread_t *)malloc(
        this->group_size * WORKERS_PER_GROUP * sizeof(pthread_t));

    if (NULL == this->barriers || NULL == this->workers) {
        perror("malloc");
        goto Error;
    }

    for (int idx = 0; idx < this->group_size; ++idx) {
        if (-1 == pthread_barrier_init(&this->barriers[idx],
                                                    NULL,
                                                    WORKERS_PER_GROUP + 1)) {
            perror("pthread_barrier_init");
            goto Error;
        }
    }

    for (int idx = 0; idx < this->group_size * WORKERS_PER_GROUP; ++idx) {
        if (-1 == pthread_create(&this->workers[idx],
                                            NULL,
                                            &start_routine,
                                            this)) {
            perror("pthread_create");
            goto Error;
        }
    }

    return 0;

Error:
    free(this->barriers);
    free(this->workers);
    pthread_cond_destroy(&this->task_available);
    pthread_cond_destroy(&this->space_available);
    pthread_cond_destroy(&this->barring_completed);
    pthread_mutex_destroy(&this->mutex_for_pool);
    pthread_mutex_destroy(&this->mutex_for_queue);

    return -1;
}

int thread_pool_run(thread_pool_t *this, void (*run)(void *), void *args) {
    if (NULL == this || NULL == run) {
        fprintf(stderr, "Null pointer exception.\n");
        return -1;
    }

    task_t task = { .run = run, .arguments = args };
    pthread_mutex_lock(&this->mutex_for_queue);

    if (is_full(&this->task_queue)) {
        pthread_cond_wait(&this->space_available, &this->mutex_for_queue);
    } 

    if (is_empty(&this->task_queue)) {
        pthread_cond_signal(&this->task_available);
    }

    if (-1 == task_queue_push(&this->task_queue, &task)) {
        fprintf(stderr, "Full queue exception.\n");
        pthread_mutex_unlock(&this->mutex_for_queue);
        return -1;
    }

    pthread_mutex_unlock(&this->mutex_for_queue);
    return 0;
}

int thread_pool_destroy(thread_pool_t *this) {
    if (NULL == this) {
        fprintf(stderr, "Null pointer exception.\n");
        return -1;
    }

    thread_pool_run(this, exit_routine, NULL);


    /* ********************************************************************** */


    pthread_mutex_lock(&this->mutex_for_pool);

    this->shutdown_barred_group = true;

    if (this->scale_up || this->scale_down) {
        pthread_cond_wait(&this->barring_completed, &this->mutex_for_pool);
    }

    pthread_mutex_unlock(&this->mutex_for_pool);


    /* ********************************************************************** */


    for (int idx = 0; idx < this->curr_barrier_size; ++idx) {
        pthread_barrier_wait(&this->barriers[idx]);
    }

    for (int idx = 0; idx < this->group_size * WORKERS_PER_GROUP; ++idx) {
        if (-1 == pthread_join(this->workers[idx], NULL)) {
            perror("pthread_join");
            return -1;
        }
    }

    for (int idx = 0; idx < this->group_size; ++idx) {
        pthread_barrier_destroy(&this->barriers[idx]);
    }

    free(this->barriers);
    free(this->workers);
    pthread_cond_destroy(&this->task_available);
    pthread_cond_destroy(&this->space_available);
    pthread_cond_destroy(&this->barring_completed);
    pthread_mutex_destroy(&this->mutex_for_pool);
    pthread_mutex_destroy(&this->mutex_for_queue);

    return 0;
}
