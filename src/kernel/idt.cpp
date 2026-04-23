// src/kernel/idt.cpp
#include "idt.h"
#include "keyboard.h"

extern void task_switch();
extern void terminal_putchar(char c);
extern void terminal_write(const char *str);
extern void task_enable_test();

extern "C" void idt_flush(uint32_t);
extern "C" uint32_t isr_stub_table[];
extern "C" void isr_syscall();

// Номер системного вызова находится в EAX
// Аргументы: EBX, ECX, EDX
extern "C" void syscall_handler()
{
    uint32_t syscall_no;
    __asm__ volatile("mov %%eax, %0" : "=r"(syscall_no));

    if (syscall_no == 1)
    {
        uint32_t str_ptr, len;
        __asm__ volatile("mov %%ebx, %0" : "=r"(str_ptr));
        __asm__ volatile("mov %%ecx, %0" : "=r"(len));
        char *s = (char *)str_ptr;
        for (uint32_t i = 0; i < len; ++i)
        {
            terminal_putchar(s[i]);
        }
    }
    // exit не предусмотрен — программа остаётся в Ring 3
}

extern "C" void fault_handler(Registers *regs)
{
    volatile int x = regs->int_no;
    (void)x;
    while (1)
    {
        __asm__("hlt");
    }
}

extern "C" void irq_handler(Registers *regs)
{
    if (regs->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
    if (regs->int_no == 33)
    {
        Keyboard::handle_interrupt();
    }
    else if (regs->int_no == 32)
    {
        // terminal_putchar('I'); // отладочный символ (можно будет убрать)
        task_switch();
    }
}

void IDT::init()
{
    // Ремаппинг PIC
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    static IDTEntry idt_entries[256];
    static IDTPtr idt_ptr;

    for (int i = 0; i < 48; ++i)
    {
        uint32_t handler = isr_stub_table[i];
        idt_entries[i].base_low = handler & 0xFFFF;
        idt_entries[i].base_high = (handler >> 16) & 0xFFFF;
        idt_entries[i].selector = 0x08;
        idt_entries[i].zero = 0;
        idt_entries[i].flags = 0x8E;
    }

    // Системный вызов (int 0x80, DPL=3)
    idt_entries[0x80].base_low = (uint32_t)&isr_syscall;
    idt_entries[0x80].base_high = 0;
    idt_entries[0x80].selector = 0x08;
    idt_entries[0x80].zero = 0;
    idt_entries[0x80].flags = 0x8E | 0x60; // Present, trap gate, DPL=3

    for (int i = 49; i < 256; ++i)
    {
        idt_entries[i].base_low = 0;
        idt_entries[i].base_high = 0;
        idt_entries[i].selector = 0;
        idt_entries[i].zero = 0;
        idt_entries[i].flags = 0;
    }

    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base = reinterpret_cast<uint32_t>(&idt_entries);
    idt_flush(reinterpret_cast<uint32_t>(&idt_ptr));
}