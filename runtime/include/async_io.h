#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/epoll.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Async IO operation type
typedef enum AsyncOpType {
    ASYNC_OP_READ,
    ASYNC_OP_WRITE
} AsyncOpType;

// Async IO operation status
typedef enum AsyncOpStatus {
    ASYNC_OP_PENDING,
    ASYNC_OP_COMPLETED,
    ASYNC_OP_ERROR
} AsyncOpStatus;

// Async IO operation struct
typedef struct AsyncOp {
#ifdef _WIN32
    HANDLE handle;
    OVERLAPPED overlapped;
#else
    int fd;
#endif
    AsyncOpType type;
    void* buffer;
    size_t size;
    size_t result;
    AsyncOpStatus status;
    int error;
    void (*callback)(void*);
    void* user_data;
} AsyncOp;

// Initialize async IO
int async_init(void);

// Cleanup async IO
void async_cleanup(void);

// Run async IO event loop
int async_run(void);

// Stop async IO event loop
void async_stop(void);

// Create async read operation
AsyncOp* async_read(int fd, void* buffer, size_t size, void (*callback)(void*), void* user_data);

// Create async write operation
AsyncOp* async_write(int fd, void* buffer, size_t size, void (*callback)(void*), void* user_data);

// Wait for async IO operation to complete
int async_wait(AsyncOp* op);

// Destroy async IO operation
void async_destroy(AsyncOp* op);

#ifdef __cplusplus
}
#endif
