// src/kernel/pmm.cpp
#include "pmm.h"

#include <stdint.h>

#include "multiboot.h"

// Константы
const size_t PAGE_SIZE = 4096;
const size_t MAX_MEMORY = 0xFFFFFFFF;  // 4 ГБ (32 бита)

// Битовая карта: каждый бит отвечает за одну страницу
static uint32_t* bitmap = nullptr;
static size_t total_pages = 0;
static size_t free_pages = 0;

// Вспомогательные функции
static inline void bitmap_set(size_t page) {
    bitmap[page / 32] |= (1 << (page % 32));
}

static inline void bitmap_clear(size_t page) {
    bitmap[page / 32] &= ~(1 << (page % 32));
}

static inline bool bitmap_test(size_t page) {
    return bitmap[page / 32] & (1 << (page % 32));
}

void pmm_init(uint32_t mmap_addr, uint32_t mmap_length) {
    // Сначала отмечаем всю память как занятую
    total_pages = MAX_MEMORY / PAGE_SIZE;
    size_t bitmap_size = total_pages / 8;  // в байтах
    // Разместим битовую карту сразу за концом ядра (пока топорно)
    extern uint8_t _kernel_end[];  // из linker.ld
    bitmap = (uint32_t*)_kernel_end;

    // Очищаем битовую карту (все заняты)
    for (size_t i = 0; i < total_pages / 32; ++i) {
        bitmap[i] = 0;
    }
    free_pages = 0;

    // Парсим карту памяти от GRUB и освобождаем доступные регионы
    multiboot_mmap_entry* entry = (multiboot_mmap_entry*)mmap_addr;
    while ((uint32_t)entry < mmap_addr + mmap_length) {
        if (entry->type == 1) {  // свободная память
            uint64_t addr = entry->base_addr;
            uint64_t len = entry->length;
            // Выравниваем по границам страниц
            uint64_t aligned_addr = (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            uint64_t aligned_len =
                (len - (aligned_addr - addr)) & ~(PAGE_SIZE - 1);

            for (uint64_t page = aligned_addr / PAGE_SIZE;
                 page < (aligned_addr + aligned_len) / PAGE_SIZE; ++page) {
                if (!bitmap_test(page)) {
                    bitmap_clear(page);  // освобождаем
                    free_pages++;
                }
            }
        }
        entry = (multiboot_mmap_entry*)((uint32_t)entry + entry->size +
                                        sizeof(uint32_t));
    }

    // Занимаем память, в которой лежит само ядро и битовая карта
    // extern uint8_t _kernel_start[]; // можно добавить в linker.ld, но мы пока
    // грубо: пометим первые N страниц занятыми (до конца битовой карты + ядро)
    uint32_t kernel_end_phys = (uint32_t)bitmap + bitmap_size;
    for (size_t page = 0; page < kernel_end_phys / PAGE_SIZE + 1; ++page) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            free_pages--;
        }
    }
}

void* pmm_alloc_page() {
    for (size_t page = 0; page < total_pages; ++page) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            free_pages--;
            return (void*)(page * PAGE_SIZE);
        }
    }
    return nullptr;  // нет памяти
}

void pmm_free_page(void* addr) {
    size_t page = (uint32_t)addr / PAGE_SIZE;
    if (bitmap_test(page)) {
        bitmap_clear(page);
        free_pages++;
    }
}

size_t pmm_free_pages_count() { return free_pages; }
