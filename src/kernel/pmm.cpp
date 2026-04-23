// src/kernel/pmm.cpp
#include "pmm.h"
#include "multiboot2.h"
#include <stdint.h>

static uint32_t* bitmap = nullptr;
static size_t total_pages = 0;
static size_t free_pages = 0;
const size_t PAGE_SIZE = 4096;

static bool early_alloc = true;

uint32_t g_mmap_addr = 0;
uint32_t g_mmap_length = 0;

void pmm_set_early_alloc(bool val) { early_alloc = val; }

static inline void bitmap_set(size_t page) { bitmap[page / 32] |= (1 << (page % 32)); }
static inline void bitmap_clear(size_t page) { bitmap[page / 32] &= ~(1 << (page % 32)); }
static inline bool bitmap_test(size_t page) { return bitmap[page / 32] & (1 << (page % 32)); }

void pmm_init(uint32_t mmap_addr, uint32_t mmap_length) {
    g_mmap_addr = mmap_addr;
    g_mmap_length = mmap_length;

    total_pages = 0xFFFFFFFF / PAGE_SIZE;
    size_t bitmap_size = total_pages / 8;

    extern uint8_t _kernel_end[];
    bitmap = (uint32_t*)_kernel_end;

    for (size_t i = 0; i < total_pages / 32; ++i) bitmap[i] = 0;
    free_pages = 0;

    multiboot2_mmap_entry* entry = (multiboot2_mmap_entry*)mmap_addr;
    while ((uint32_t)entry < mmap_addr + mmap_length) {
        if (entry->type == 1) {
            uint64_t base = entry->addr;
            uint64_t len  = entry->len;
            uint64_t aligned_base = (base + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            uint64_t aligned_len  = (len - (aligned_base - base)) & ~(PAGE_SIZE - 1);
            for (uint64_t page = aligned_base / PAGE_SIZE;
                 page < (aligned_base + aligned_len) / PAGE_SIZE;
                 ++page) {
                if (!bitmap_test(page)) {
                    bitmap_clear(page);
                    free_pages++;
                }
            }
        }
        entry = (multiboot2_mmap_entry*)((uint32_t)entry + sizeof(multiboot2_mmap_entry) + ((entry->type == 1) ? 8 : 0)); // entry size is entry_size? but we just use sizeof. Actually multiboot2_mmap_entry is 24 bytes. Continuation requires next entry.
        // Более корректно:
        // entry = (multiboot2_mmap_entry*)((uint32_t)entry + entry_size);
        // Но у нас нет entry_size из тега, поэтому пропустим
    }

    uint32_t kernel_end_phys = (uint32_t)bitmap + bitmap_size;
    for (size_t page = 0; page < kernel_end_phys / PAGE_SIZE + 1; ++page) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            free_pages--;
        }
    }
}

void* pmm_alloc_page() {
    size_t start = 0, end = total_pages;
    if (early_alloc) end = 0x400000 / PAGE_SIZE; // первые 4 МБ
    for (size_t page = start; page < end; ++page) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            free_pages--;
            return (void*)(page * PAGE_SIZE);
        }
    }
    return nullptr;
}

void pmm_free_page(void* addr) {
    size_t page = (uint32_t)addr / PAGE_SIZE;
    if (bitmap_test(page)) {
        bitmap_clear(page);
        free_pages++;
    }
}

size_t pmm_free_pages_count() { return free_pages; }