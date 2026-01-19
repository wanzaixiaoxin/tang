#pragma once

#include <stddef.h>
#include <stdint.h>

// 运行时库主头文件
// 包含所有核心组件的声明

// 协程相关
#include "coroutine.h"

// 调度器相关
#include "scheduler.h"

// 异步IO相关
#include "async_io.h"

// 线程管理相关
#include "thread.h"

// 初始化运行时
void tang_runtime_init();

// 清理运行时
void tang_runtime_cleanup();

// 启动主事件循环
int tang_runtime_run();
