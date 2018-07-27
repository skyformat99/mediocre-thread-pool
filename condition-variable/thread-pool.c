#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "thread-pool.h"

#define exit_routine (void *)-1L    // 0xffffffffffffffff

static void *start_routine(void *args) {
    bool done = false;
    task_t task = { 0 };
    thread_pool_t *this = args;

    while (! done) {
        pthread_mutex_lock(&this->mutex);

        while (is_empty(&this->task_queue)) {
            pthread_cond_wait(&this->task_available, &this->mutex);
        }

        if (is_full(&this->task_queue)) {
            pthread_cond_signal(&this->space_available);
        }

        if (-1 == task_queue_pop(&this->task_queue, &task)) {
            fprintf(stderr, "Empty queue exception.\n");
            pthread_mutex_unlock(&this->mutex);
            pthread_exit(NULL);
        }

        pthread_mutex_unlock(&this->mutex);
        (task.run == exit_routine) ? done = true : task.run(task.arguments);
    }
    
    pthread_exit(NULL);
}

int thread_pool_init(thread_pool_t *this, const int size) {
    if (NULL == this) {
        fprintf(stderr, "Null pointer exception.\n");
        return -1;
    }

    if (0 >= size) {
        fprintf(stderr, "Invalid size of thread pool.\n");
        return -1;
    }

    task_queue_init(&this->task_queue);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);

    if (-1 == pthread_mutex_init(&this->mutex, &attr)) {
        perror("pthread_mutex_init");
        return -1;
    }

    pthread_mutexattr_destroy(&attr);

    if (-1 == pthread_cond_init(&this->task_available, NULL) ||
        -1 == pthread_cond_init(&this->space_available, NULL)) {
        perror("pthread_cond_init");
        return -1;
    }

    this->size = size;
    this->workers = (pthread_t *)malloc(this->size * sizeof(pthread_t));
    if (NULL == this->workers) {
        perror("malloc");
        return -1;
    }

    for (int tid = 0; tid < this->size; ++tid) {
        if (-1 == pthread_create(&this->workers[tid],
                                                NULL,
                                                &start_routine,
                                                this)) {
            perror("pthread_create");
            free(this->workers);
            pthread_mutex_destroy(&this->mutex);
            pthread_cond_destroy(&this->task_available);
            pthread_cond_destroy(&this->space_available);
            return -1;
        }
    }

    return 0;
}

int thread_pool_run(thread_pool_t *this, void (*run)(void *), void *args) {
    if (NULL == this || NULL == run) {
        fprintf(stderr, "Null pointer exception.\n");
        return -1;
    }

    task_t task = { .run = run, .arguments = args };
    pthread_mutex_lock(&this->mutex);
    
    if (is_full(&this->task_queue)) {
        pthread_cond_wait(&this->space_available, &this->mutex);
    }

    if (-1 == task_queue_push(&this->task_queue, &task)) {
        fprintf(stderr, "Full queue exception.\n");
        pthread_mutex_unlock(&this->mutex);
        return -1;
    }

    pthread_cond_signal(&this->task_available);
    pthread_mutex_unlock(&this->mutex);

    return 0;
}

int thread_pool_destroy(thread_pool_t *this) {
    if (NULL == this) {
        fprintf(stderr, "Null pointer exception.\n");
        return -1;
    }

    for (int tid = 0; tid < this->size; ++tid) {
        thread_pool_run(this, exit_routine, NULL);
    }

    for (int tid = 0; tid < this->size; ++tid) {
        if (-1 == pthread_join(this->workers[tid], NULL)) {
            perror("pthread_join");
            return -1;
        }
    }

    free(this->workers);
    pthread_mutex_destroy(&this->mutex);
    pthread_cond_destroy(&this->task_available);
    pthread_cond_destroy(&this->space_available);

    return 0;
}
