#include <stdio.h>
#include <stdlib.h>
#include "../runtime/include/coroutine.h"
#include "../runtime/include/scheduler.h"

// Coroutine function
void coroutine_func(void* arg) {
    int id = *(int*)arg;
    printf("Coroutine %d started\n", id);
    
    for (int i = 0; i < 3; i++) {
        printf("Coroutine %d yielding %d\n", id, i);
        coro_yield();
    }
    
    printf("Coroutine %d finished\n", id);
}

int main() {
    printf("Starting coroutine test\n");
    
    // Initialize scheduler
    Sched* scheduler = sched_create(2); // 2 worker threads
    if (!scheduler) {
        printf("Failed to create scheduler\n");
        return 1;
    }
    
    // Create coroutines
    Coro* coro1 = NULL;
    Coro* coro2 = NULL;
    
    int id1 = 1;
    int id2 = 2;
    
    coro1 = coro_create(coroutine_func, &id1, 4096);
    coro2 = coro_create(coroutine_func, &id2, 4096);
    
    if (!coro1 || !coro2) {
        printf("Failed to create coroutines\n");
        return 1;
    }
    
    printf("Created coroutines\n");
    
    // Add coroutines to scheduler
    sched_add(scheduler, coro1);
    sched_add(scheduler, coro2);
    
    // Run scheduler
    sched_run(scheduler);
    
    printf("All coroutines finished\n");
    
    // Destroy coroutines
    coro_destroy(coro1);
    coro_destroy(coro2);
    
    // Cleanup scheduler
    sched_destroy(scheduler);
    
    printf("Coroutine test completed\n");
    return 0;
}
