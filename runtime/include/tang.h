#pragma once

#include <stddef.h>

// Coroutine status
typedef enum TangCoroStatus {
    TANG_CORO_READY,
    TANG_CORO_RUNNING,
    TANG_CORO_SUSPENDED,
    TANG_CORO_FINISHED
} TangCoroStatus;

// Coroutine struct
typedef struct TangCoro {
    void* stack_base;
    void* stack_top;
    size_t stack_size;
    void* context;
    TangCoroStatus status;
    void* user_data;
    void (*entry)(void*);
    void* result;
} TangCoro;

// Scheduler struct
typedef struct TangScheduler {
    TangCoro** run_queue;
    size_t run_queue_size;
    size_t run_queue_capacity;
    int num_workers;
    void** workers;
    int stop;
    void* mutex;
    void* events;
} TangScheduler;

// Coroutine functions
TangCoro* tang_coro_create(void (*entry)(void*), void* user_data, size_t stack_size);
int tang_coro_resume(TangCoro* coro);
int tang_coro_yield(void);
void tang_coro_destroy(TangCoro* coro);
TangCoro* tang_coro_current(void);
TangCoroStatus tang_coro_get_status(const TangCoro* coro);

// Scheduler functions
TangScheduler* tang_scheduler_create(int num_workers);
void tang_scheduler_destroy(TangScheduler* scheduler);
int tang_scheduler_add_coro(TangScheduler* scheduler, TangCoro* coro);
int tang_scheduler_run(TangScheduler* scheduler);
void tang_scheduler_stop(TangScheduler* scheduler);

// Async IO functions
int tang_async_init(void);
void tang_async_cleanup(void);
int tang_async_run(void);
void tang_async_stop(void);
