#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#include IMPL


#ifdef SYNC_TEST
#define NUM_OF_REQUESTS 10000

_Atomic int cnt;

void task(void *args) {
    atomic_fetch_add_explicit(&cnt, 1, memory_order_relaxed);
}

#else
#define NUM_OF_REQUESTS 1000000

static double diff_in_second(struct timespec t1, struct timespec t2) {
    struct timespec diff;

    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }

    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

void task(void *args) {
    // Spend a millisecond to execute an I/O bound task.
    usleep(1000);
}

#endif


int main(int argc, char const *argv[]) {
    // fprintf(stderr, "%d\n", getpid());
    // sleep(20);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <#threads>\n", argv[0]);
        return -1;
    }

    thread_pool_t thrpool;
    int size = atoi(argv[1]);


#ifdef WORK_GROUP
    size = size / WORKERS_PER_GROUP;

#endif


    if (-1 == thread_pool_init(&thrpool, size)) {
        fprintf(stderr, "Failed to initialize the thread pool.\n");
        return -1;
    }


#ifndef SYNC_TEST
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

#endif


    int requests = NUM_OF_REQUESTS;

    while (requests--) {
        if (-1 == thread_pool_run(&thrpool, &task, NULL)) {
            fprintf(stderr, "Failed to run a task.\n");
            return -1;
        }
    }

    if (-1 == thread_pool_destroy(&thrpool)) {
        fprintf(stderr, "Failed to destroy a thread pool.\n");
        return -1;
    }


#ifdef SYNC_TEST
    printf("%s\n", (cnt == NUM_OF_REQUESTS) ? "PASS" : "FAIL");

#else
    clock_gettime(CLOCK_REALTIME, &end);

    FILE *out = fopen("result/statistics.txt", "a");
    if (! out) {
        perror("fopen");
        return -1;
    }

    double requests_per_second = NUM_OF_REQUESTS / diff_in_second(start, end);
    printf("requests per second: %.2lf\n", requests_per_second);
    fprintf(out, "%lf\n", requests_per_second);
    fclose(out);

#endif


    return 0;
}
