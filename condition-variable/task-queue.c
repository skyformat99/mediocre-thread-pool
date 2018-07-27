#include "task-queue.h"

inline int size(task_queue_t *this) {
    return (this->front < this->rear)
        ? this->rear - this->front
        : (RING_QUEUE_CAPACITY + 1) - (this->front - this->rear);
}

inline bool is_full(task_queue_t *this) {
    return this->front == (this->rear + 1) % (RING_QUEUE_CAPACITY + 1);
}

inline bool is_empty(task_queue_t *this) {
    return this->rear == this->front;
}

inline void task_queue_init(task_queue_t *this) {
    this->rear = 0;
    this->front = 0;
}

inline int task_queue_pop(task_queue_t *this, task_t *task_ptr) {
    if (is_empty(this)) {
        return -1;
    }

    *task_ptr = this->queue[this->front];
    this->front = (this->front + 1) % (RING_QUEUE_CAPACITY + 1);

    return 0;
}

inline int task_queue_push(task_queue_t *this, task_t *task_ptr) {
    if (is_full(this)) {
        return -1;
    }

    this->queue[this->rear] = *task_ptr;
    this->rear = (this->rear + 1) % (RING_QUEUE_CAPACITY + 1);

    return 0;
}
