// src/kernel/task.cpp
#include "task.h"

extern void terminal_putchar(char c);
extern uint8_t terminal_color; // из kernel.cpp
extern "C" void switch_to_task(TaskContext *old_ctx, TaskContext *new_ctx);

static uint8_t test_stack[4096] __attribute__((aligned(16)));

static Task main_task;
static Task test_task;
static Task *current_task = &main_task;

void test_thread()
{
    // Меняем цвет текста циклически
    uint8_t colors[] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    int idx = 0;
    while (1)
    {
        terminal_color = colors[idx % 6];
        terminal_putchar('T');
        for (volatile int i = 0; i < 5000000; ++i)
            ;
        idx++;
    }
}

void tasking_init()
{
    main_task.active = true;
    main_task.stack = nullptr;
    current_task = &main_task;

    test_task.active = true;
    test_task.stack = test_stack;

    uint32_t *sp = (uint32_t *)(test_stack + sizeof(test_stack) - 16);
    *(--sp) = 0x202;
    *(--sp) = 0x08;
    *(--sp) = (uint32_t)&test_thread;
    sp -= 8;
    test_task.ctx.esp = (uint32_t)sp;
    test_task.ctx.ebp = 0;
    test_task.ctx.eip = 0;
    test_task.ctx.eflags = 0;
}

void task_switch()
{
    if (!test_task.active)
        return;
    Task *old = current_task;
    current_task = (current_task == &main_task) ? &test_task : &main_task;
    switch_to_task(&old->ctx, &current_task->ctx);
}

void task_disable_test()
{
    test_task.active = false;
}

void task_enable_test()
{
    test_task.active = true;
}