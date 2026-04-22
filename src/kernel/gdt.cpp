// src/kernel/gdt.cpp
#include "gdt.h"

// Мы создадим 3 записи: нулевая (обязательно), код ядра, данные ядра
static GDTEntry gdt_entries[3];
static GDTPtr gdt_ptr;

// Внешняя ассемблерная функция для загрузки GDT и обновления сегментных
// регистров
extern "C" void gdt_flush(uint32_t);

void GDT::init() {
    // 1. Нулевой дескриптор (процессор требует)
    gdt_entries[0].limit_low = 0;
    gdt_entries[0].base_low = 0;
    gdt_entries[0].base_middle = 0;
    gdt_entries[0].access = 0;
    gdt_entries[0].granularity = 0;
    gdt_entries[0].base_high = 0;

    // 2. Сегмент кода ядра (кольцо 0)
    gdt_entries[1].limit_low = 0xFFFF;
    gdt_entries[1].base_low = 0x0000;
    gdt_entries[1].base_middle = 0x00;
    gdt_entries[1].access =
        0x9A;  // 10011010b: Present, Ring 0, Code, Exec/Read
    gdt_entries[1].granularity =
        0xCF;  // 11001111b: 32-bit, 4K granularity, limit high=0xF
    gdt_entries[1].base_high = 0x00;

    // 3. Сегмент данных ядра (кольцо 0)
    gdt_entries[2].limit_low = 0xFFFF;
    gdt_entries[2].base_low = 0x0000;
    gdt_entries[2].base_middle = 0x00;
    gdt_entries[2].access =
        0x92;  // 10010010b: Present, Ring 0, Data, Read/Write
    gdt_entries[2].granularity = 0xCF;
    gdt_entries[2].base_high = 0x00;

    // Указатель для lgdt
    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = reinterpret_cast<uint32_t>(&gdt_entries);

    // Загружаем GDT и обновляем сегментные регистры
    gdt_flush(reinterpret_cast<uint32_t>(&gdt_ptr));
}
