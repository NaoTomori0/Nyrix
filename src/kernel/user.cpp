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
    // Простой тест: выводим символ '!' через syscall
    const char msg = '!!!!';

    // Устанавливаем регистры для syscall
    uint32_t eax = 1;              // syscall номер 1 = write
    uint32_t ebx = (uint32_t)&msg; // указатель на символ
    uint32_t ecx = 1;              // длина = 1

    // Копируем в глобальные переменные (так как int $0x80 не передаёт регистры)
    syscall_eax = eax;
    syscall_ebx = ebx;
    syscall_ecx = ecx;

    // Вызываем прерывание
    __asm__ volatile("int $0x80");

    // Оставаемся в Ring 3
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
    __asm__ volatile(
        "cli\n\t"
        "mov %0, %%esp\n\t"
        "iret\n\t"
        : : "r"(sp));

    // Сюда попадём только после exit
}
