#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>

#include "thread-pool.h"

// worker thread starts from here.
static void *start_routine(void *args) {
    int nbytes = 0;
    task_t task = { 0 };
    thread_pool_t *this = args;

    while (1) {
        pthread_mutex_lock(&this->mutex);

        if (this->shutdown) {
            pthread_mutex_unlock(&this->mutex);
            break;
        }
        
        // Return zero indicates the boss thread has been closed write end
        // pipe, in other words there are no tasks that need to be executed.
        // Therefore, worker thread should exit function.
        nbytes = read(this->pipefd[0], &task, sizeof(task));

        if (0 == nbytes) {
            this->shutdown = true;
            pthread_mutex_unlock(&this->mutex);
            break;
        } else if (-1 == nbytes) {
            perror("read");
        }

        pthread_mutex_unlock(&this->mutex);
        task.run(task.arguments);
    }

    pthread_exit(NULL);
}

int thread_pool_init(thread_pool_t *this, const int size) {
    if (0 >= size) {
        fprintf(stderr, "Invalid size of thread pool.\n");
        return -1;
    }

    if (NULL == this) {
        fprintf(stderr, "Null pointer exception.\n");
        return -1;
    }

    this->shutdown = false;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);

    if (-1 == pthread_mutex_init(&this->mutex, &attr)) {
        perror("pthread_mutex_init");
        return -1;
    }

    pthread_mutexattr_destroy(&attr);

    if (-1 == pipe(this->pipefd)) {
        perror("pipe");
        pthread_mutex_destroy(&this->mutex);
        return -1;
    }

    this->size = size;
    this->workers = (pthread_t *)malloc(this->size * sizeof(pthread_t));

    for (int idx = 0; idx < this->size; ++idx) {
        if (-1 == pthread_create(&this->workers[idx],
                                                NULL,
                                                &start_routine,
                                                this)) {
            perror("pthread_create");
            free(this->workers);
            close(this->pipefd[0]);
            close(this->pipefd[1]);
            pthread_mutex_destroy(&this->mutex);
            return -1;
        }
    }

    return 0;
}

int thread_pool_run(thread_pool_t *this, void (*run)(void *), void *args) {
    task_t task = { .run = run, .arguments = args };

    if (-1 == write(this->pipefd[1], &task, sizeof(task_t))) {
        perror("write");
        return -1;
    }

    return 0;
}

int thread_pool_destroy(thread_pool_t *this) {
    // Close the write end pipe to notify worker threads that no tasks need to
    // be executed.
    close(this->pipefd[1]);

    for (int idx = 0; idx < this->size; ++idx) {
        if (-1 == pthread_join(this->workers[idx], NULL)) {
            perror("pthread_join");
            break;
        }
    }

    free(this->workers);
    close(this->pipefd[0]);
    pthread_mutex_destroy(&this->mutex);

    return 0;
}
