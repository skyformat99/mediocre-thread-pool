#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_

#define RING_QUEUE_CAPACITY 4096

#include <stdbool.h>

typedef struct __TASK_TAG__ {
    void (*run)(void *);
    void *arguments;

} task_t;

typedef struct __TASK_QUEUE_TAG__ {
    int front;
    int rear;
    task_t queue[RING_QUEUE_CAPACITY + 1];

} task_queue_t;

int size(task_queue_t *this);

bool is_full(task_queue_t *this);

bool is_empty(task_queue_t *this);

void task_queue_init(task_queue_t *this);

int task_queue_pop(task_queue_t *this, task_t *task_ptr);

int task_queue_push(task_queue_t *this, task_t *task_ptr);

#endif /* TASK_QUEUE_H_ */
