#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <stdbool.h>
#include <pthread.h>


// Description:
//      Declare the task interface that can be executed by a worker thread.
typedef struct __TASK_TAG__ {
    void (*run)(void *);
    void *arguments;

} task_t;


// Description:
//      Thread pool structure.
// 
// Attributes:
//      shutdown:
//          If true, the worker threads is terminated.
//      size:
//          Number of the worker threads.
//      workers:
//          Dynamically allocate 1-dim pthread array.
//      pipefd[0]:
//          The worker threads reads the task from the blocking pipe.
//      pipefd[1]:
//          The boss thread writes the task to the blocking pipe.
//      mutex:
//          The worker threads compete with each other for the right to use the
//          read end pipe.
typedef struct __THREAD_POOl_TAG__ {
    bool shutdown;
    int size;
    int pipefd[2];
    pthread_mutex_t mutex;
    pthread_t *workers;

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
//      If the boss thread attempts to write to a full pipe, then function
//      blocks until sufficient data has been read from the pipe to allow the
//      write to complete.
// 
//      If necessary, fcntl() can be used to change the pipe capacity.
int thread_pool_run(thread_pool_t *this, void (*run)(void *), void *args);


// Description:
//      Join the worker threads and release resources.
//
// Return value:
//      Return zero on success, or -1 if an error occurred.
int thread_pool_destroy(thread_pool_t *this);


#endif /* THREAD_POOL_H_ */
