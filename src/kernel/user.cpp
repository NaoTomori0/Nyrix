// src/kernel/user.cpp
#include "user.h"
#include <stdint.h>
#include "gfx.h"
#include "idt.h"

__attribute__((aligned(16))) static uint8_t user_stack[16384];
extern uint32_t kernel_esp;
extern uint32_t return_eip;

// Глобальные переменные для системных вызовов
extern uint32_t syscall_eax;
extern uint32_t syscall_ebx;
extern uint32_t syscall_ecx;

// Прямой вывод в отладочный порт
static inline void debug_putchar(char c)
{
    __asm__ volatile("out %%al, $0xE9" : : "a"(c));
}

void user_task()
{
    debug_putchar('U');
    debug_putchar('\n');
    
    // Просто выводим символ и ждём
    debug_putchar('W');
    debug_putchar('A');
    debug_putchar('I');
    debug_putchar('T');
    debug_putchar('\n');
    
    // Зависаем на hlt - это безопасно в Ring 3
    while (1)
        __asm__ volatile("hlt");
}

void switch_to_user_mode()
{
    debug_putchar('1');
    debug_putchar('\n');
    
    // 1. Сохраняем стек ядра
    __asm__ volatile("mov %%esp, %0" : "=r"(kernel_esp));
    debug_putchar('2');
    debug_putchar('\n');

    // 2. Устанавливаем пользовательские сегменты данных
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));
    debug_putchar('3');
    debug_putchar('\n');

    // 3. Готовим стек для iret
    uint32_t user_stack_top = (uint32_t)(&user_stack[sizeof(user_stack)]);
    uint32_t *sp = (uint32_t *)(user_stack_top);
    
    // Стек для IRET в правильном порядке: SS, ESP, EFLAGS, CS, EIP
    *(--sp) = 0x23;                 // SS (пользовательский сегмент данных)
    *(--sp) = user_stack_top;       // ESP (вершина стека пользователя)
    *(--sp) = 0x202;                // EFLAGS (IF флаг установлен)
    *(--sp) = 0x1B;                 // CS (пользовательский сегмент кода)
    *(--sp) = (uint32_t)&user_task; // EIP
    debug_putchar('4');
    debug_putchar('\n');

    // 4. Сохраняем адрес возврата
    __asm__ volatile("mov $1f, %0\n" : "=r"(return_eip) : : "memory");
    __asm__ volatile("1:");
    debug_putchar('5');
    debug_putchar('\n');

    // 5. Переход в Ring 3
    uint32_t esp_val = (uint32_t)sp;
    debug_putchar('X');
    debug_putchar('\n');
    
    __asm__ volatile(
        "mov %0, %%esp\n\t"
        "iret\n\t"
        : : "r"(esp_val) : "memory");

    debug_putchar('K');
    debug_putchar('\n');
}
