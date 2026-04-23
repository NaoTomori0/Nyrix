// src/kernel/paging.cpp
#include "paging.h"

#include "idt.h"  // для outb/inb, если нужно (но здесь не используются)
#include "pmm.h"

// Размеры
const size_t PAGE_SIZE_4K = 4096;
const size_t PAGE_TABLE_ENTRIES = 1024;
const size_t PAGE_DIRECTORY_ENTRIES = 1024;

// Биты флагов
const uint32_t PAGE_PRESENT = 0x1;
const uint32_t PAGE_WRITABLE = 0x2;
const uint32_t PAGE_USER = 0x4;  // будет использоваться позже

// Каталог страниц и первая таблица (ядерная)
static uint32_t* page_directory =
    nullptr;  // физический адрес? Мы будем хранить вирт.
static uint32_t* first_page_table = nullptr;

void paging_init() {
    // 1. Выделяем физическую страницу под каталог
    void* pd_phys = pmm_alloc_page();
    if (!pd_phys) return;  // критично, но пока без паники
    page_directory = reinterpret_cast<uint32_t*>(pd_phys);

    // 2. Выделяем физическую страницу под первую таблицу (первые 4 МБ)
    void* pt_phys = pmm_alloc_page();
    if (!pt_phys) return;
    first_page_table = reinterpret_cast<uint32_t*>(pt_phys);

    // 3. Заполняем первую таблицу: identity mapping для 0..4 МБ
    for (size_t i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
        uint32_t phys_addr = i * PAGE_SIZE_4K;
        first_page_table[i] = phys_addr | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // 4. Очищаем весь каталог (все записи сброшены в 0 = отсутствуют)
    for (size_t i = 0; i < PAGE_DIRECTORY_ENTRIES; ++i) {
        page_directory[i] = 0;
    }

    // 5. Устанавливаем первую запись в каталоге (охватывает 0..4 МБ)
    // Адрес таблицы должен быть физическим, флаги
    uint32_t pt_phys_addr = reinterpret_cast<uint32_t>(pt_phys);
    page_directory[0] = pt_phys_addr | PAGE_PRESENT | PAGE_WRITABLE;

    // 6. Загружаем адрес каталога в CR3
    uint32_t pd_phys_addr = reinterpret_cast<uint32_t>(pd_phys);
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys_addr));

    // 7. Включаем paging (установка бита PG в CR0, и WP для защиты)
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // PG (31-й бит)
    cr0 |= 0x00010000;  // WP (16-й бит) – запрещает запись в read-only страницы
                        // в ring0
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
