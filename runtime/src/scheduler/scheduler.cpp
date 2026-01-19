#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../include/scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

static Sched* current_sched = NULL;

#ifdef _WIN32
DWORD WINAPI sched_worker(LPVOID arg) {
    Sched* sched = (Sched*)arg;
    (void)sched; // Not used yet
    return 0;
}
#else
void* sched_worker(void* arg) {
    Sched* sched = (Sched*)arg;
    (void)sched; // Not used yet
    return NULL;
}
#endif

Sched* sched_create(int num_workers) {
    if (num_workers <= 0) {
        num_workers = 4; // Default 4 worker threads
    }

    Sched* sched = (Sched*)malloc(sizeof(Sched));
    if (!sched) {
        return NULL;
    }

    sched->run_queue_capacity = 1024;
    sched->run_queue_size = 0;
    sched->run_queue = (Coro**)malloc(sizeof(Coro*) * sched->run_queue_capacity);
    if (!sched->run_queue) {
        free(sched);
        return NULL;
    }

    sched->num_workers = num_workers;
    sched->workers = (void**)malloc(sizeof(void*) * num_workers);
    if (!sched->workers) {
        free(sched->run_queue);
        free(sched);
        return NULL;
    }

    sched->stop = 0;

#ifdef _WIN32
    // Create mutex
    sched->mutex = CreateMutex(NULL, FALSE, NULL);
    if (!sched->mutex) {
        free(sched->workers);
        free(sched->run_queue);
        free(sched);
        return NULL;
    }

    // Create worker event array
    sched->worker_events = (HANDLE*)malloc(sizeof(HANDLE) * num_workers);
    if (!sched->worker_events) {
        CloseHandle(sched->mutex);
        free(sched->workers);
        free(sched->run_queue);
        free(sched);
        return NULL;
    }

    for (int i = 0; i < num_workers; ++i) {
        sched->worker_events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!sched->worker_events[i]) {
            for (int j = 0; j < i; ++j) {
                CloseHandle(sched->worker_events[j]);
            }
            free(sched->worker_events);
            CloseHandle(sched->mutex);
            free(sched->workers);
            free(sched->run_queue);
            free(sched);
            return NULL;
        }
    }
#else
    // Initialize mutex and condition variable
    pthread_mutex_init(&sched->mutex, NULL);
    pthread_cond_init(&sched->cond, NULL);
#endif

    current_sched = sched;
    return sched;
}

void sched_destroy(Sched* sched) {
    if (!sched) {
        return;
    }

    sched_stop(sched);

#ifdef _WIN32
    // Close mutex
    CloseHandle(sched->mutex);

    // Close worker events
    if (sched->worker_events) {
        for (int i = 0; i < sched->num_workers; ++i) {
            CloseHandle(sched->worker_events[i]);
        }
        free(sched->worker_events);
    }
#else
    // Destroy mutex and condition variable
    pthread_mutex_destroy(&sched->mutex);
    pthread_cond_destroy(&sched->cond);
#endif

    if (sched->run_queue) {
        free(sched->run_queue);
    }

    if (sched->workers) {
        free(sched->workers);
    }

    free(sched);

    if (current_sched == sched) {
        current_sched = NULL;
    }
}

int sched_add(Sched* sched, Coro* coro) {
    if (!sched || !coro) {
        return -1;
    }

#ifdef _WIN32
    // Wait for mutex
    if (WaitForSingleObject(sched->mutex, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }
#else
    // Lock mutex
    pthread_mutex_lock(&sched->mutex);
#endif

    // Check if run queue is full
    if (sched->run_queue_size >= sched->run_queue_capacity) {
        size_t new_cap = sched->run_queue_capacity * 2;
        Coro** new_queue = (Coro**)realloc(sched->run_queue, sizeof(Coro*) * new_cap);
        if (!new_queue) {
#ifdef _WIN32
            ReleaseMutex(sched->mutex);
#else
            pthread_mutex_unlock(&sched->mutex);
#endif
            return -1;
        }
        sched->run_queue = new_queue;
        sched->run_queue_capacity = new_cap;
    }

    // Add coroutine to run queue
    sched->run_queue[sched->run_queue_size++] = coro;

#ifdef _WIN32
    // Release mutex
    ReleaseMutex(sched->mutex);
    
    // Trigger first available worker event
    for (int i = 0; i < sched->num_workers; ++i) {
        if (SetEvent(sched->worker_events[i])) {
            break;
        }
    }
#else
    // Notify one waiting worker
    pthread_cond_signal(&sched->cond);
    
    // Release mutex
    pthread_mutex_unlock(&sched->mutex);
#endif

    return 0;
}

int sched_run(Sched* sched) {
    if (!sched) {
        return -1;
    }

#ifdef _WIN32
    // Create worker threads
    DWORD* thread_ids = (DWORD*)malloc(sizeof(DWORD) * sched->num_workers);
    if (!thread_ids) {
        return -1;
    }

    for (int i = 0; i < sched->num_workers; ++i) {
        sched->workers[i] = CreateThread(NULL, 0, sched_worker, sched, 0, &thread_ids[i]);
        if (!sched->workers[i]) {
            // Cleanup already created threads
            for (int j = 0; j < i; ++j) {
                CloseHandle(sched->workers[j]);
            }
            free(thread_ids);
            return -1;
        }
    }

    // Wait for worker threads to complete
    for (int i = 0; i < sched->num_workers; ++i) {
        WaitForSingleObject(sched->workers[i], INFINITE);
        CloseHandle(sched->workers[i]);
    }

    free(thread_ids);
#else
    pthread_t* threads = (pthread_t*)sched->workers;
    for (int i = 0; i < sched->num_workers; ++i) {
        if (pthread_create(&threads[i], NULL, sched_worker, sched) != 0) {
            return -1;
        }
    }

    // Wait for worker threads to complete
    for (int i = 0; i < sched->num_workers; ++i) {
        pthread_join(threads[i], NULL);
    }
#endif

    return 0;
}

void sched_stop(Sched* sched) {
    if (!sched) {
        return;
    }

    sched->stop = 1;

#ifdef _WIN32
    // Wake up all worker threads
    for (int i = 0; i < sched->num_workers; ++i) {
        SetEvent(sched->worker_events[i]);
    }
#else
    // Lock mutex
    pthread_mutex_lock(&sched->mutex);
    
    // Wake up all waiting worker threads
    pthread_cond_broadcast(&sched->cond);
    
    // Release mutex
    pthread_mutex_unlock(&sched->mutex);
#endif
}

Sched* sched_current(void) {
    return current_sched;
}

#ifdef __cplusplus
}
#endif
