#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <pthread.h>
#include "task-queue.h"


// Description:
//      Thread pool structure.
//
// Attributes:
//      size:
//          Number of the worker threads.
//      workers:
//          Dynamically allocate 1-dim pthread array.
//      space_available:
//          Block the boss thread until task queue is not full.
//      task_available:
//          Block the worker threads until task queue is not empty.
//      task_queue:
//          The boss thread inserts task to the queue and the worker threads
//          gets task from the task queue.
//      mutex:
//          The boss thread compete with the worker threads for the right to use
//          the task queue.
typedef struct __THREAD_POOl_TAG__ {
    int size;
    pthread_t *workers;
    pthread_mutex_t mutex;
    pthread_cond_t task_available;
    pthread_cond_t space_available;
    task_queue_t task_queue;

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
int thread_pool_init(thread_pool_t *this, const int size);


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
