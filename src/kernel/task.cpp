// src/kernel/task.cpp
#include "task.h"

extern void terminal_putchar(char c);
extern uint8_t terminal_color;
extern "C" void switch_to_task(TaskContext *old_ctx, TaskContext *new_ctx);

static uint8_t test_stack[4096] __attribute__((aligned(16)));

static Task main_task;
static Task test_task;
static Task *current_task = &main_task;

void test_thread()
{
    while (1)
    {
        terminal_putchar('T');
        terminal_color = (terminal_color == 0x0F) ? 0x1F : 0x0F;
        for (volatile int i = 0; i < 5000000; ++i)
            ;
    }
}

void tasking_init()
{
    // test_thread();
    // Инициализируем контекст главного потока (мы сейчас в нём)
    // Сохраним текущий указатель стека как отправную точку
    // switch_to_task при первом вызове сохранит актуальные регистры в old_ctx,
    // поэтому заполнять их здесь не нужно.
    main_task.active = true;
    main_task.stack = nullptr; // главный поток использует стек ядра
    current_task = &main_task;

    // Настраиваем тестовую задачу
    test_task.active = true;
    test_task.stack = test_stack;

    uint32_t *sp = (uint32_t *)(test_stack + sizeof(test_stack) - 16);
    *(--sp) = 0x202;                  // EFLAGS (IF=1)
    *(--sp) = 0x08;                   // CS (код ядра)
    *(--sp) = (uint32_t)&test_thread; // EIP
    sp -= 8;                          // место для pushad
    test_task.ctx.esp = (uint32_t)sp;
    test_task.ctx.ebp = 0; // не используется при старте
    test_task.ctx.eip = 0;
    test_task.ctx.eflags = 0;
}

void task_switch()
{
    if (!test_task.active)
        return;

    // Переключаемся: текущая -> другая
    Task *old = current_task;
    current_task = (current_task == &main_task) ? &test_task : &main_task;
    switch_to_task(&old->ctx, &current_task->ctx);
}