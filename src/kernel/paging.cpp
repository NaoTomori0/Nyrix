// src/kernel/paging.cpp
#include "paging.h"
#include "pmm.h"
#include "multiboot2.h"
#include <stdint.h>

extern uint32_t g_mmap_addr;
extern uint32_t g_mmap_length;

const uint32_t PAGE_PRESENT = 1;
const uint32_t PAGE_WRITABLE = 2;
const uint32_t PAGE_USER = 4;

static uint32_t *page_directory = nullptr;
static uint32_t next_pt = 0x202000; // продолжаем выделять таблицы после первых 4 МБ

void paging_map_framebuffer(uint64_t addr, uint64_t size)
{
    if (!page_directory)
        return;
    uint64_t aligned_base = addr & ~0xFFF;
    uint64_t aligned_end = (addr + size + 0xFFF) & ~0xFFF;
    for (uint64_t phys = aligned_base; phys < aligned_end; phys += 0x1000)
    {
        uint32_t pd_idx = (phys >> 22) & 0x3FF;
        uint32_t pt_idx = (phys >> 12) & 0x3FF;
        if (!(page_directory[pd_idx] & PAGE_PRESENT))
        {
            uint32_t *pt = (uint32_t *)next_pt;
            next_pt += 0x1000;
            for (int i = 0; i < 1024; ++i)
                pt[i] = 0;
            page_directory[pd_idx] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        }
        uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & ~0xFFF);
        pt[pt_idx] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }
}

void paging_init_full()
{
    // 1. Каталог страниц (2 МБ)
    page_directory = (uint32_t *)0x200000;
    for (int i = 0; i < 1024; ++i)
        page_directory[i] = 0;

    // 2. Первая таблица – первые 4 МБ (с флагом USER)
    uint32_t *first_pt = (uint32_t *)0x201000;
    for (int i = 0; i < 1024; ++i)
        first_pt[i] = (i * 4096) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    page_directory[0] = (uint32_t)first_pt | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    // 3. Обработка карты памяти (начиная с >4 МБ)
    multiboot2_mmap_entry *entry = (multiboot2_mmap_entry *)g_mmap_addr;
    while ((uint32_t)entry < g_mmap_addr + g_mmap_length)
    {
        if (entry->type == 1)
        {
            uint64_t base = entry->addr;
            uint64_t len = entry->len;
            uint64_t aligned_base = (base + 0xFFF) & ~0xFFF;
            uint64_t aligned_len = (len - (aligned_base - base)) & ~0xFFF;
            if (aligned_base < 0x400000)
            {
                if (aligned_base + aligned_len <= 0x400000)
                {
                    entry = (multiboot2_mmap_entry *)((uint32_t)entry + sizeof(multiboot2_mmap_entry));
                    continue;
                }
                aligned_len -= (0x400000 - aligned_base);
                aligned_base = 0x400000;
            }
            for (uint64_t phys = aligned_base; phys < aligned_base + aligned_len; phys += 0x1000)
            {
                uint32_t pd_idx = (phys >> 22) & 0x3FF;
                uint32_t pt_idx = (phys >> 12) & 0x3FF;
                if (!(page_directory[pd_idx] & PAGE_PRESENT))
                {
                    uint32_t *pt = (uint32_t *)next_pt;
                    next_pt += 0x1000;
                    for (int i = 0; i < 1024; ++i)
                        pt[i] = 0;
                    page_directory[pd_idx] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
                }
                uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & ~0xFFF);
                pt[pt_idx] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
            }
        }
        entry = (multiboot2_mmap_entry *)((uint32_t)entry + sizeof(multiboot2_mmap_entry));
    }

    // 4. Добавляем флаг USER на ВСЕ страницы в нижних 4 МБ (0x100000..0x400000)
    for (uint32_t addr = 0x100000; addr < 0x400000; addr += 0x1000)
    {
        uint32_t pd_idx = (addr >> 22) & 0x3FF;
        uint32_t pt_idx = (addr >> 12) & 0x3FF;
        if (page_directory[pd_idx] & PAGE_PRESENT)
        {
            uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & ~0xFFF);
            if (pt[pt_idx] & PAGE_PRESENT)
            {
                pt[pt_idx] |= PAGE_USER;
            }
        }
    }

    // 5. Включение пейджинга
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    cr0 |= 0x00010000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("ljmp $0x08, $1f\n1:\n");
}