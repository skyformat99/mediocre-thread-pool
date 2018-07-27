#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#define WORK_GROUP 1
#define WORKERS_PER_GROUP 64

#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include "task-queue.h"


// Description:
//      The worker threads first compete with each other and then compete with
//      the boss thread.
//
//      Increase the probability that the boss thread uses the task queue,
//      thereby reducing the number of times the context switch occurs because
//      the task queue is empty.
//
//      The number of worker threads can be dynamically scaled up or scaled down
//      depending on how busy the current system is.
//
// Attributes:
//      shutdown_pool:
//          If true, non-barred work groups is terminated.
//      shutdown_barred_group:
//          If true, barred work groups is terminated.
//      group_size:
//          Number of work groups.
//      workers:
//          Dynamically allocate 1-dim pthread array.
//      space_available:
//          Block the boss thread until task queue is not full.
//      task_available:
//          Block a worker thread until task queue is not empty.
//      barring_completed:
//          Block the boss thread until the work group completes
//          synchronization.
//      mutex_for_queue:
//          The boss thread compete with "a" worker thread for the task queue.
//      mutex_for_pool:
//          The worker threads compete with each other for the right to use the
//          task queue.
//      task_queue:
//          The boss thread inserts task to the queue and the worker threads
//          gets task from the task queue.
//      scale_up:
//          If true, wake up a sleeping work group.
//      scale_down:
//          If true, transfer a work group to the kernel waiting queue.
//      barriers:
//          Synchronize a work group at a barrier. The number of syncers is
//          equal to the number of work groups.
//      eval_cnt:
//          Thread pool performance is evaluated every time N tasks are
//          performed.
//      waiting_workers:
//          Record the number of worker threads waiting for mutex_for_pool.
//      curr_barrier_size:
//          Currently, the number of synchronized work groups.
//      curr_barred_workers:
//          Count until the number of barred the worker threads is equal to
//          workers per group .
typedef struct __THREAD_POOl_TAG__ {
    bool scale_up;
    bool scale_down;
    bool shutdown_pool;
    bool shutdown_barred_group;

    int eval_cnt;
    int group_size;
    int curr_barrier_size;
    int curr_barred_workers;

    _Atomic int waiting_workers;

    pthread_t *workers;
    pthread_barrier_t *barriers;
    task_queue_t task_queue;

    pthread_cond_t task_available;
    pthread_cond_t space_available;
    pthread_cond_t barring_completed;

    pthread_mutex_t mutex_for_pool;
    pthread_mutex_t mutex_for_queue;
    
} thread_pool_t;


// Description:
//      Initializes the thread pool with the specified values.
//
// Example:
//     thread_pool_t thrpool;
//     thread_pool_init(&thrpool, 8);
//
// Return value:
//      Return zero on success, or -1 if an error occurred.
int thread_pool_init(thread_pool_t *this, const int group_size);


// Description:
//      The boss thread inserts the task into the task queue. The task waiting
//      for the worker thread to execute.
//
// Example:
//      void foo(void *str) { ... }
//      thread_pool_run(&thrpool, &foo, "Hello World");
//
// Return value:
//      Return zero on success, or -1 if an error occurred.
//
// Note:
//      If the boss thread attempts to insert a task to a full task queue, then
//      function blocks until sufficient data has been got from the task queue
//      to allow the insert to complete.
int thread_pool_run(thread_pool_t *this, void (*run)(void *), void *args);


// Description:
//      Join the worker threads and release resources.
//
// Return value:
//      Return zero on success, or -1 if an error occurred.
int thread_pool_destroy(thread_pool_t *this);


#endif /* THREAD_POOL_H_ */
