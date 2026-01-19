#include <stdlib.h>
#include <stdio.h>
#include "../include/tang.h"

static TangCoro* current_coro = NULL;
static TangScheduler* current_scheduler = NULL;

// Coroutine implementation
TangCoro* tang_coro_create(void (*entry)(void*), void* user_data, size_t stack_size) {
    if (stack_size == 0) {
        stack_size = 1024 * 1024; // Default 1MB stack size
    }

    TangCoro* coro = (TangCoro*)malloc(sizeof(TangCoro));
    if (!coro) {
        return NULL;
    }

    coro->stack_base = malloc(stack_size);
    if (!coro->stack_base) {
        free(coro);
        return NULL;
    }

    coro->stack_size = stack_size;
    coro->stack_top = (char*)coro->stack_base + stack_size;
    coro->context = coro->stack_top;
    coro->status = TANG_CORO_READY;
    coro->user_data = user_data;
    coro->entry = entry;
    coro->result = NULL;

    return coro;
}

int tang_coro_resume(TangCoro* coro) {
    if (!coro || coro->status == TANG_CORO_FINISHED) {
        return -1;
    }

    TangCoro* prev = current_coro;
    current_coro = coro;

    if (coro->status == TANG_CORO_READY) {
        coro->status = TANG_CORO_RUNNING;
        coro->entry(coro->user_data);
        coro->status = TANG_CORO_FINISHED;
    }

    current_coro = prev;
    return 0;
}

int tang_coro_yield(void) {
    if (!current_coro) {
        return -1;
    }

    current_coro->status = TANG_CORO_SUSPENDED;
    return 0;
}

void tang_coro_destroy(TangCoro* coro) {
    if (!coro) {
        return;
    }

    if (coro->stack_base) {
        free(coro->stack_base);
    }

    free(coro);
}

TangCoro* tang_coro_current(void) {
    return current_coro;
}

TangCoroStatus tang_coro_get_status(const TangCoro* coro) {
    if (!coro) {
        return TANG_CORO_FINISHED;
    }
    return coro->status;
}

// Scheduler implementation
TangScheduler* tang_scheduler_create(int num_workers) {
    if (num_workers <= 0) {
        num_workers = 4;
    }

    TangScheduler* scheduler = (TangScheduler*)malloc(sizeof(TangScheduler));
    if (!scheduler) {
        return NULL;
    }

    scheduler->run_queue_capacity = 1024;
    scheduler->run_queue_size = 0;
    scheduler->run_queue = (TangCoro**)malloc(sizeof(TangCoro*) * scheduler->run_queue_capacity);
    if (!scheduler->run_queue) {
        free(scheduler);
        return NULL;
    }

    scheduler->num_workers = num_workers;
    scheduler->workers = (void**)malloc(sizeof(void*) * num_workers);
    if (!scheduler->workers) {
        free(scheduler->run_queue);
        free(scheduler);
        return NULL;
    }

    scheduler->stop = 0;
    scheduler->mutex = NULL;
    scheduler->events = NULL;

    current_scheduler = scheduler;
    return scheduler;
}

void tang_scheduler_destroy(TangScheduler* scheduler) {
    if (!scheduler) {
        return;
    }

    tang_scheduler_stop(scheduler);

    if (scheduler->run_queue) {
        free(scheduler->run_queue);
    }

    if (scheduler->workers) {
        free(scheduler->workers);
    }

    free(scheduler);

    if (current_scheduler == scheduler) {
        current_scheduler = NULL;
    }
}

int tang_scheduler_add_coro(TangScheduler* scheduler, TangCoro* coro) {
    if (!scheduler || !coro) {
        return -1;
    }

    // Simplified implementation: directly add to run queue
    if (scheduler->run_queue_size >= scheduler->run_queue_capacity) {
        size_t new_cap = scheduler->run_queue_capacity * 2;
        TangCoro** new_queue = (TangCoro**)realloc(scheduler->run_queue, sizeof(TangCoro*) * new_cap);
        if (!new_queue) {
            return -1;
        }
        scheduler->run_queue = new_queue;
        scheduler->run_queue_capacity = new_cap;
    }

    scheduler->run_queue[scheduler->run_queue_size++] = coro;
    return 0;
}

int tang_scheduler_run(TangScheduler* scheduler) {
    if (!scheduler) {
        return -1;
    }

    // Simplified implementation: directly execute all coroutines
    for (size_t i = 0; i < scheduler->run_queue_size; i++) {
        tang_coro_resume(scheduler->run_queue[i]);
    }

    return 0;
}

void tang_scheduler_stop(TangScheduler* scheduler) {
    if (scheduler) {
        scheduler->stop = 1;
    }
}

// Async IO implementation
int tang_async_init(void) {
    return 0;
}

void tang_async_cleanup(void) {
    // Simplified implementation: no resources to clean up
}

int tang_async_run(void) {
    return 0;
}

void tang_async_stop(void) {
    // Simplified implementation: no need to stop
}
