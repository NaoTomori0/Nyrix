#ifndef NYRIX_TASK_H
#define NYRIX_TASK_H

#include <stdint.h>

struct TaskContext
{
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
};

struct Task
{
    TaskContext ctx;
    uint8_t *stack;
    uint32_t pgd; // физический адрес Page Directory
    bool active;
    bool is_user; // true = пользовательская задача
    Task *parent; // куда возвращаться после exit
};

void tasking_init();
void task_switch();
void task_disable_test();
void task_enable_test();
uint32_t create_address_space();

// Запускает пользовательскую задачу: переключает контекст и выполняет iret
void start_user_task(Task *task);

// Завершает текущую пользовательскую задачу и возвращается к parent
void task_exit();

extern Task *current_task; // глобальная текущая задача

#endif