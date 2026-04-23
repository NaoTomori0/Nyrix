// src/kernel/user.cpp
#include "user.h"
#include <stdint.h>
#include "idt.h" // для inb/outb

extern uint8_t terminal_color;
extern void task_disable_test();

__attribute__((aligned(16))) static uint8_t user_stack[16384];

void user_task()
{
    uint16_t *vga = (uint16_t *)0xB8000;
    while (1)
    {
        // Инвертируем атрибут цвета для первых 80 символов (первая строка)
        for (int i = 0; i < 80; ++i)
        {
            uint8_t attr = (vga[i] >> 8) & 0xFF;
            attr = ~attr;
            vga[i] = (attr << 8) | ' ';
        }
        for (volatile int i = 0; i < 5000000; ++i)
            ;
    }
}

void switch_to_user_mode()
{
    // 1. Останавливаем всё, что может помешать
    __asm__ volatile("cli");
    // Маскируем IRQ0 (таймер) в ведущем PIC
    uint8_t mask = inb(0x21);
    outb(0x21, mask | 0x01);
    // Отключаем переключение задач
    task_disable_test();

    // 2. Устанавливаем пользовательские сегменты данных
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));

    // 3. Готовим стек для iret (IF=0 в EFLAGS)
    uint32_t *sp = (uint32_t *)(&user_stack[sizeof(user_stack) - sizeof(uint32_t)]);
    *(--sp) = 0x23;                 // SS
    *(--sp) = (uint32_t)(sp + 1);   // ESP
    *(--sp) = 0x002;                // EFLAGS (IF=0)
    *(--sp) = 0x1B;                 // CS
    *(--sp) = (uint32_t)&user_task; // EIP

    // 4. Переход в Ring 3
    __asm__ volatile(
        "mov %0, %%esp\n"
        "iret\n"
        :
        : "r"(sp));
}