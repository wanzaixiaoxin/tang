#pragma once

#include <stddef.h>
#include "coroutine.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Scheduler struct
typedef struct Sched {
    Coro** run_queue;
    size_t run_queue_size;
    size_t run_queue_capacity;
    int num_workers;
    void** workers;
    int stop;
    
#ifdef _WIN32
    HANDLE mutex;
    HANDLE* worker_events;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
} Sched;

// Initialize scheduler
Sched* sched_create(int num_workers);

// Cleanup scheduler
void sched_destroy(Sched* sched);

// Add coroutine to run queue
int sched_add(Sched* sched, Coro* coro);

// Run scheduler
int sched_run(Sched* sched);

// Stop scheduler
void sched_stop(Sched* sched);

// Get current scheduler
Sched* sched_current(void);

#ifdef __cplusplus
}
#endif
