// src/kernel/user.cpp
#include "user.h"
#include <stdint.h>
#include "gfx.h"
#include "idt.h"

__attribute__((aligned(16))) static uint8_t user_stack[16384];
extern uint32_t kernel_esp;
extern uint32_t return_eip;
extern void serial_putchar(char c);

void user_task()
{
    __asm__ volatile("outb %0, %1" : : "a"('U'), "Nd"(0xE9));
    while (1)
        ;
}

void switch_to_user_mode()
{
    // 1. Сохраняем стек ядра
    __asm__ volatile("mov %%esp, %0" : "=r"(kernel_esp));

    // 2. Сохраняем текущую маску IRQ (необязательно отключать)
    uint8_t old_mask = inb(0x21);

    // 3. Устанавливаем пользовательские сегменты данных
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));

    // 4. Готовим стек для iret
    uint32_t *sp = (uint32_t *)(&user_stack[sizeof(user_stack)]);

    // Стек для IRET в порядке: EIP, CS, EFLAGS, ESP, SS
    *(--sp) = 0x23;                 // SS (пользовательский сегмент данных)
    *(--sp) = (uint32_t)(sp + 2);   // ESP (указатель стека пользователя)
    *(--sp) = 0x202;                // EFLAGS (IF флаг установлен, бит 1 зарезервирован)
    *(--sp) = 0x1B;                 // CS (пользовательский сегмент кода)
    *(--sp) = (uint32_t)&user_task; // EIP

    // 5. Сохраняем адрес возврата (для будущего exit)
    __asm__ volatile("mov $1f, %0\n" : "=r"(return_eip) : : "memory");
    __asm__ volatile("1:");

    // 6. Переход в Ring 3
    __asm__ volatile(
        "cli\n\t"
        "mov %0, %%esp\n\t"
        "iret\n\t"
        : : "r"(sp));

    // Сюда попадём только после exit (пока не используется)
}