// src/kernel/idt.cpp
#include "idt.h"

#include "keyboard.h"
#include "paging.h"

static IDTEntry idt_entries[256];
static IDTPtr idt_ptr;

// Таблица указателей на заглушки прерываний (определена в ассемблере)
extern "C" uint32_t isr_stub_table[];

// Внешняя ассемблерная функция для загрузки IDT
extern "C" void idt_flush(uint32_t);

// Обработчики исключений и прерываний (пока простые)
extern "C" void fault_handler(Registers* regs) {
    if (regs->int_no == 14) {
        uint32_t fault_addr;
        __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
        // Можно записать в VGA напрямую, но чтобы не засорять, просто запретим
        // прерывания и зависнем В будущем здесь будет логирование
        (void)fault_addr;
        // Для отладки можно моргнуть курсором, но пока просто бесконечный цикл
        while (1) {
            __asm__("hlt");
        }
    }
    // Остальные исключения
    while (1) {
        __asm__("hlt");
    }
}

extern "C" void irq_handler(Registers* regs) {
    // Отправляем EOI (End of Interrupt) в контроллер прерываний
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);  // ведомый PIC
    }
    outb(0x20, 0x20);  // ведущий PIC

    // Здесь в будущем будет диспетчеризация по драйверам
    if (regs->int_no == 33) {  // IRQ1 = клавиатура
        Keyboard::handle_interrupt();
    }
}

void IDT::init() {
    // Перенастройка PIC: сдвигаем IRQ0-15 на вектора 32-47
    outb(0x20, 0x11);  // ICW1: начать инициализацию, каскадный режим
    outb(0xA0, 0x11);
    outb(0x21, 0x20);  // ICW2: базовый вектор ведущего = 32
    outb(0xA1, 0x28);  // базовый вектор ведомого = 40
    outb(0x21, 0x04);  // ICW3: ведущий подключен к IRQ2
    outb(0xA1, 0x02);  // ведомый идентификатор
    outb(0x21, 0x01);  // ICW4: режим 8086
    outb(0xA1, 0x01);
    outb(0x21, 0x0);  // маскирование: пока все разрешены
    outb(0xA1, 0x0);

    // Заполняем первые 48 векторов (исключения + IRQ)
    for (int i = 0; i < 48; ++i) {
        uint32_t handler = isr_stub_table[i];
        idt_entries[i].base_low = handler & 0xFFFF;
        idt_entries[i].base_high = (handler >> 16) & 0xFFFF;
        idt_entries[i].selector = 0x08;  // сегмент кода ядра
        idt_entries[i].zero = 0;
        idt_entries[i].flags = 0x8E;  // Present, 32-bit interrupt gate, Ring 0
    }

    // Остальные вектора не инициализированы
    for (int i = 48; i < 256; ++i) {
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
