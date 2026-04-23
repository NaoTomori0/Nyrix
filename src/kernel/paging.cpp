// src/kernel/paging.cpp
#include "paging.h"
#include <stdint.h>

// Статические структуры (выровнены по 4K в секции .bss)
__attribute__((aligned(4096)))
static uint32_t page_directory[1024];

__attribute__((aligned(4096)))
static uint32_t first_page_table[1024];

void paging_init_simple() {
    // 1. Identity mapping первых 4 МБ (0-4MB)
    for (uint32_t i = 0; i < 1024; ++i) {
        first_page_table[i] = (i * 4096) | 0x3; // Present + Writable
    }

    // 2. Очищаем каталог
    for (uint32_t i = 0; i < 1024; ++i) {
        page_directory[i] = 0;
    }

    // 3. Первая запись каталога указывает на нашу таблицу
    page_directory[0] = (uint32_t)first_page_table | 0x3;

    // 4. Загружаем физический адрес каталога в CR3
    uint32_t pd_phys = (uint32_t)page_directory;
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys));

    // 5. Включаем paging в CR0
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // PG
    cr0 |= 0x00010000; // WP
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    // 6. Сброс CS (дальний прыжок) для синхронизации конвейера
    // Делаем это инлайн-ассемблером через ljmp
    __asm__ volatile(
        "ljmp $0x08, $1f\n"
        "1:\n"
    );
}