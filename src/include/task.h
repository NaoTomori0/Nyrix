// src/include/task.h
#ifndef NYRIX_TASK_H
#define NYRIX_TASK_H

#include <stdint.h>

struct TaskContext {
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
};

struct Task {
    TaskContext ctx;
    uint8_t* stack;
    bool active;
};

void tasking_init();
void task_switch();
void task_enable_test();
void task_disable_test();

#endif
