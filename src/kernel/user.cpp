// src/kernel/user.cpp
#include "user.h"
#include <stdint.h>
#include "gfx.h"
#include "idt.h"

__attribute__((aligned(16))) static uint8_t user_stack[16384];
extern uint32_t kernel_esp;
extern uint32_t return_eip;
extern void serial_putchar(char c);

// Глобальные переменные для системных вызовов
extern uint32_t syscall_eax;
extern uint32_t syscall_ebx;
extern uint32_t syscall_ecx;

void user_task()
{
    // Простой тест: выводим символ в Ring 3
    serial_putchar('U');  // Выведет 'U' если мы в Ring 3
    
    // Уходим обратно в ядро через int 0x80
    uint32_t eax = 0;  // syscall 0 = exit
    syscall_eax = eax;
    
    // Вызываем прерывание
    __asm__ volatile("int $0x80");
    
    // Если вернулись - зависаем
    while (1)
        __asm__ volatile("hlt");
}

void switch_to_user_mode()
{
    // 1. Сохраняем стек ядра
    __asm__ volatile("mov %%esp, %0" : "=r"(kernel_esp));

    // 2. Устанавливаем пользовательские сегменты данных
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));

    // 3. Готовим стек для iret
    uint32_t user_stack_top = (uint32_t)(&user_stack[sizeof(user_stack)]);
    uint32_t *sp = (uint32_t *)(user_stack_top);
    
    // Стек для IRET в правильном порядке: SS, ESP, EFLAGS, CS, EIP
    *(--sp) = 0x23;                 // SS (пользовательский сегмент данных)
    *(--sp) = user_stack_top;       // ESP (вершина стека пользователя)
    *(--sp) = 0x202;                // EFLAGS (IF флаг установлен)
    *(--sp) = 0x1B;                 // CS (пользовательский сегмент кода)
    *(--sp) = (uint32_t)&user_task; // EIP

    // 4. Сохраняем адрес возврата (для будущего exit)
    __asm__ volatile("mov $1f, %0\n" : "=r"(return_eip) : : "memory");
    __asm__ volatile("1:");

    // 5. Переход в Ring 3
    // ИСПРАВЛЕНИЕ: Убираем cli - iret сам восстановит IF из EFLAGS
    __asm__ volatile(
        "mov %0, %%esp\n\t"
        "iret\n\t"
        : : "r"(sp));

    // Сюда попадём только после exit
}
