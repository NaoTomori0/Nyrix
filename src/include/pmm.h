#ifndef NYRIX_PMM_H
#define NYRIX_PMM_H

#include <stdint.h>
#include <stddef.h>

void pmm_init(uint32_t mmap_addr, uint32_t mmap_length);
void* pmm_alloc_page();
void pmm_free_page(void* addr);
size_t pmm_free_pages_count();

// Новый вызов
void pmm_set_early_alloc(bool val);

#endif