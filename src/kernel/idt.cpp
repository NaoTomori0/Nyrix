// src/kernel/idt.cpp
#include "idt.h"
#include "keyboard.h"
#include "gfx.h"

extern void task_switch();
extern "C" void isr_syscall();
extern "C" void idt_flush(uint32_t);
extern "C" uint32_t isr_stub_table[];
extern uint32_t return_esp;

extern uint32_t syscall_eax;
extern uint32_t syscall_ebx;
extern uint32_t syscall_ecx;
extern uint32_t kernel_esp;
extern uint32_t return_eip;
extern bool use_polling; // из kernel.cpp
extern void serial_putchar(char c);

extern "C" void syscall_handler()
{
    uint32_t syscall_no = syscall_eax;

    if (syscall_no == 1)
    { // write
        const char *str = (const char *)syscall_ebx;
        uint32_t len = syscall_ecx;
        for (uint32_t i = 0; i < len; ++i)
        {
            serial_putchar(str[i]); // вывод в COM1
        }
    }
    // exit больше не нужен
}

extern "C" void fault_handler(Registers *regs)
{
    serial_putchar('E');                       // признак исключения
    serial_putchar('0' + (regs->int_no / 10)); // десятки
    serial_putchar('0' + (regs->int_no % 10)); // единицы
    while (1)
    {
        __asm__("hlt");
    }
}

extern "C" void irq_handler(Registers *regs)
{
    if (regs->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
    if (regs->int_no == 33)
    {
        Keyboard::handle_interrupt();
    }
    else if (regs->int_no == 32)
    {
        task_switch();
    }
}
void IDT::init()
{
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
        idt_entries[i].flags = 0x8E; // Present, DPL=0, interrupt gate
    }

    // Системный вызов (int 0x80) – trap gate, DPL=3
    idt_entries[0x80].base_low = (uint32_t)&isr_syscall & 0xFFFF;
    idt_entries[0x80].base_high = ((uint32_t)&isr_syscall >> 16) & 0xFFFF;
    idt_entries[0x80].selector = 0x08;
    idt_entries[0x80].zero = 0;
    idt_entries[0x80].flags = 0xEF; // trap gate, DPL=3
    // Остальные вектора (начиная с 0x81, не трогаем 0x80!)
    for (int i = 0x81; i < 256; ++i)
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