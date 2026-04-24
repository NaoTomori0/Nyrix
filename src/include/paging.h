// src/include/paging.h
#ifndef NYRIX_PAGING_H
#define NYRIX_PAGING_H

#include <stdint.h>
void paging_init_full();
void paging_init_simple();
void paging_init();
void paging_init_simple();
void page_fault_handler(uint32_t error_code, uint32_t fault_addr);
void paging_map_framebuffer(uint64_t addr, uint64_t size);

#endif
