#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../include/coroutine.h"

#ifdef __cplusplus
extern "C" {
#endif

static Coro* current = NULL;

Coro* coro_create(void (*entry)(void*), void* user_data, size_t stack_size) {
    if (stack_size == 0) {
        stack_size = 1024 * 1024; // 1MB default stack
    }

    Coro* coro = (Coro*)malloc(sizeof(Coro));
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
    coro->context = NULL; // Simplified: remove context pointer
    coro->status = CORO_STATUS_READY;
    coro->user_data = user_data;
    coro->entry = entry;
    coro->result = NULL;

    return coro;
}

int coro_resume(Coro* coro) {
    if (!coro || coro->status == CORO_STATUS_FINISHED) {
        return -1;
    }

    Coro* prev = current;
    current = coro;

    if (coro->status == CORO_STATUS_READY) {
        coro->status = CORO_STATUS_RUNNING;
        coro->entry(coro->user_data);
        coro->status = CORO_STATUS_FINISHED;
    }

    current = prev;
    return 0;
}

int coro_yield(void) {
    if (!current) {
        return -1;
    }

    current->status = CORO_STATUS_SUSPENDED;
    return 0;
}

void coro_destroy(Coro* coro) {
    if (!coro) {
        return;
    }

    if (coro->stack_base) {
        free(coro->stack_base);
    }

    free(coro);
}

Coro* coro_current(void) {
    return current;
}

CoroStatus coro_status(const Coro* coro) {
    if (!coro) {
        return CORO_STATUS_FINISHED;
    }
    return coro->status;
}

// Remove assembly dependency, simplified context switch function
void coro_swap(void** from, void* to) {
    (void)from;
    (void)to;
}

#ifdef __cplusplus
}
#endif
