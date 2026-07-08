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
    serial_putchar('\n');
    
    // Уходим обратно в ядро через int 0x80
    uint32_t eax = 0;  // syscall 0 = exit
    syscall_eax = eax;
    
    serial_putchar('C');  // О сейчас вызовем int 0x80
    serial_putchar('A');
    serial_putchar('L');
    serial_putchar('L');
    serial_putchar('\n');
    
    // Вызываем прерывание
    __asm__ volatile("int $0x80");
    
    serial_putchar('R');  // О вернулись из int 0x80
    serial_putchar('E');
    serial_putchar('T');
    serial_putchar('\n');
    
    // Если вернулись - зависаем
    while (1)
        __asm__ volatile("hlt");
}

void switch_to_user_mode()
{
    serial_putchar('1');  // Отладка: входим в switch_to_user_mode
    serial_putchar('\n');
    
    // 1. Сохраняем стек ядра
    __asm__ volatile("mov %%esp, %0" : "=r"(kernel_esp));
    serial_putchar('2');
    serial_putchar('\n');

    // 2. Устанавливаем пользовательские сегменты данных
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));
    serial_putchar('3');
    serial_putchar('\n');

    // 3. Готовим стек для iret
    uint32_t user_stack_top = (uint32_t)(&user_stack[sizeof(user_stack)]);
    uint32_t *sp = (uint32_t *)(user_stack_top);
    
    // Стек для IRET в правильном порядке: SS, ESP, EFLAGS, CS, EIP
    *(--sp) = 0x23;                 // SS (пользовательский сегмент данных)
    *(--sp) = user_stack_top;       // ESP (вершина стека пользователя)
    *(--sp) = 0x202;                // EFLAGS (IF флаг установлен)
    *(--sp) = 0x1B;                 // CS (пользовательский сегмент кода)
    *(--sp) = (uint32_t)&user_task; // EIP
    serial_putchar('4');
    serial_putchar('\n');

    // 4. Сохраняем адрес возврата (для будущего exit)
    __asm__ volatile("mov $1f, %0\n" : "=r"(return_eip) : : "memory");
    __asm__ volatile("1:");
    serial_putchar('5');
    serial_putchar('\n');

    // 5. Переход в Ring 3 БЕЗ cli -让iret自己处理
    // Используем встроенную функцию asm для полного контроля над состоянием процессора
    uint32_t esp_val = (uint32_t)sp;
    __asm__ volatile(
        "mov %0, %%esp\n\t"      // Загружаем новый ESP (указывает на стек iret)
        "iret\n\t"               // Прыгаем в Ring 3, восстанавливая все флаги из EFLAGS
        : : "r"(esp_val) : "memory");

    // По��ле возврата из Ring 3
    serial_putchar('K');  // Kernel mode restored
    serial_putchar('\n');
}
