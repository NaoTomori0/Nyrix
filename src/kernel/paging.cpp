// src/kernel/paging.cpp
#include "paging.h"
#include "pmm.h"
#include "multiboot.h"
#include <stdint.h>

// Внешние переменные из pmm.cpp (заполняются в pmm_init)
extern uint32_t g_mmap_addr;
extern uint32_t g_mmap_length;

static uint32_t* page_directory = nullptr;
const uint32_t PAGE_PRESENT = 1;
const uint32_t PAGE_WRITABLE = 2;

// ---------- статический вариант (первые 4 МБ) ----------
__attribute__((aligned(4096)))
static uint32_t simple_pd[1024];
__attribute__((aligned(4096)))
static uint32_t simple_pt[1024];

void paging_init_simple() {
    for (uint32_t i = 0; i < 1024; ++i) {
        simple_pt[i] = (i * 4096) | PAGE_PRESENT | PAGE_WRITABLE;
    }
    for (uint32_t i = 0; i < 1024; ++i) {
        simple_pd[i] = 0;
    }
    simple_pd[0] = (uint32_t)simple_pt | PAGE_PRESENT | PAGE_WRITABLE;

    uint32_t pd_phys = (uint32_t)simple_pd;
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // PG
    cr0 |= 0x00010000;  // WP
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    // Сброс конвейера
    __asm__ volatile("ljmp $0x08, $1f\n1:\n");
}

// ---------- полный вариант (вся память) ----------
void paging_init_full() {
    // Ранний режим: выделяем страницы только из первых 4 МБ
    pmm_set_early_alloc(true);

    // 1. Каталог страниц
    page_directory = (uint32_t*)pmm_alloc_page();
    for (int i = 0; i < 1024; ++i) {
        page_directory[i] = 0;
    }

    // 2. Перебираем карту памяти
    multiboot_mmap_entry* entry = (multiboot_mmap_entry*)g_mmap_addr;
    while ((uint32_t)entry < g_mmap_addr + g_mmap_length) {
        if (entry->type == 1) { // доступная память
            uint64_t base = entry->base_addr;
            uint64_t len  = entry->length;
            uint64_t aligned_base = (base + 0xFFF) & ~0xFFF;
            uint64_t aligned_len  = (len - (aligned_base - base)) & ~0xFFF;

            for (uint64_t phys = aligned_base; phys < aligned_base + aligned_len; phys += 0x1000) {
                uint32_t pd_idx = (phys >> 22) & 0x3FF;
                uint32_t pt_idx = (phys >> 12) & 0x3FF;

                if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
                    uint32_t* pt = (uint32_t*)pmm_alloc_page(); // из <4 МБ
                    for (int i = 0; i < 1024; ++i) pt[i] = 0;
                    page_directory[pd_idx] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE;
                }

                uint32_t* pt = (uint32_t*)(page_directory[pd_idx] & ~0xFFF);
                pt[pt_idx] = phys | PAGE_PRESENT | PAGE_WRITABLE;
            }
        }
        // следующая запись mmap
        entry = (multiboot_mmap_entry*)((uint32_t)entry + entry->size + sizeof(uint32_t));
    }

    // 3. Загружаем каталог в CR3 и включаем paging
    uint32_t pd_phys = (uint32_t)page_directory;
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("ljmp $0x08, $1f\n1:\n");

    // 4. Отключаем ранний режим, теперь PMM может выдавать любые страницы
    pmm_set_early_alloc(false);
}