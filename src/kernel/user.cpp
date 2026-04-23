// src/kernel/user.cpp
#include "user.h"
#include <stdint.h>

__attribute__((aligned(16))) static uint8_t user_stack[16384];

void user_task()
{
    const char *msg = "Hello from Ring 3!\n";
    while (1)
    {
        __asm__ volatile("int $0x80" : : "a"(1), "b"(msg), "c"(18));
        for (volatile int i = 0; i < 10000000; ++i)
            ;
    }
}

void switch_to_user_mode()
{
    // Запрещаем прерывания
    __asm__ volatile("cli");

    // Устанавливаем пользовательские сегменты
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));

    // Готовим стек для iret (используем ядерное адресное пространство)
    uint32_t *sp = (uint32_t *)(&user_stack[sizeof(user_stack) - sizeof(uint32_t)]);
    *(--sp) = 0x23;                 // SS
    *(--sp) = (uint32_t)(sp + 1);   // ESP
    *(--sp) = 0x002;                // EFLAGS (IF=0)
    *(--sp) = 0x1B;                 // CS
    *(--sp) = (uint32_t)&user_task; // EIP

    // Переход в Ring 3 (CR3 не меняем!)
    __asm__ volatile(
        "mov %0, %%esp\n"
        "iret\n"
        :
        : "r"(sp));
}