// src/kernel/user.cpp
#include "user.h"
#include <stdint.h>
#include "gfx.h"
#include "idt.h"

__attribute__((aligned(16))) static uint8_t user_stack[16384];
extern uint32_t kernel_esp;
extern uint32_t return_eip;
extern void serial_putchar(char c); // <-- добавлено

void user_task()
{
    // Ничего не делаем, только бесконечный цикл
    while (1)
        ;
}

void switch_to_user_mode()
{
    // 1. Сохраняем стек ядра
    __asm__ volatile("mov %%esp, %0" : "=r"(kernel_esp));

    // 2. Маскируем IRQ0 (таймер)
    uint8_t old_mask = inb(0x21);
    __asm__ volatile("cli");
    outb(0x21, old_mask | 0x01);

    // 3. Устанавливаем пользовательские сегменты данных
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));

    // 4. Готовим стек для iret
    uint32_t *sp = (uint32_t *)(&user_stack[sizeof(user_stack) - sizeof(uint32_t)]);
    *(--sp) = 0x23;              // SS
    sp[-1] = (uint32_t)(sp + 1); // ESP
    sp -= 1;
    *(--sp) = 0x002;                // EFLAGS (IF=0)
    *(--sp) = 0x1B;                 // CS
    *(--sp) = (uint32_t)&user_task; // EIP

    // 5. Сохраняем адрес возврата (для будущего exit)
    __asm__ volatile("mov $1f, %0\n" : "=r"(return_eip) : : "memory");
    __asm__ volatile("1:");

    // 6. Переход в Ring 3
    __asm__ volatile("mov %0, %%esp\niret" : : "r"(sp));

    // Сюда попадём только после exit (пока не используется)
}