#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Coroutine status
typedef enum CoroStatus {
    CORO_STATUS_READY,
    CORO_STATUS_RUNNING,
    CORO_STATUS_SUSPENDED,
    CORO_STATUS_FINISHED
} CoroStatus;

// Coroutine struct
typedef struct Coro {
    void* stack_base;
    void* stack_top;
    size_t stack_size;
    void* context;
    CoroStatus status;
    void* user_data;
    void (*entry)(void*);
    void* result;
} Coro;

// Create coroutine
Coro* coro_create(void (*entry)(void*), void* user_data, size_t stack_size);

// Resume coroutine execution
int coro_resume(Coro* coro);

// Suspend current coroutine
int coro_yield(void);

// Destroy coroutine
void coro_destroy(Coro* coro);

// Get current coroutine
Coro* coro_current(void);

// Get coroutine status
CoroStatus coro_status(const Coro* coro);

// Context switch function
void coro_swap(void** from, void* to);

#ifdef __cplusplus
}
#endif
