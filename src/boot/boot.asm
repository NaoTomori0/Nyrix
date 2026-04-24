; src/boot/boot.asm
bits 32

MULTIBOOT2_HEADER_MAGIC equ 0xE85250D6
ARCHITECTURE_I386       equ 0
HEADER_LENGTH           equ (multiboot2_header_end - multiboot2_header)
CHECKSUM                equ -(MULTIBOOT2_HEADER_MAGIC + ARCHITECTURE_I386 + HEADER_LENGTH)

section .multiboot
align 8
multiboot2_header:
    dd MULTIBOOT2_HEADER_MAGIC
    dd ARCHITECTURE_I386
    dd HEADER_LENGTH
    dd CHECKSUM

    ; Тег фреймбуфера (type=5)
    align 8
    dw 5                     ; тип
    dw 0                     ; флаги
    dd 20                    ; размер тега
    dd 0                     ; ширина (авто)
    dd 0                     ; высота
    dd 0                     ; глубина (авто)

    ; Тег карты памяти (type=6)
    align 8
    dw 6
    dw 0
    dd 16
    dd 0
    dd 0

    ; Завершающий тег
    align 8
    dw 0
    dw 0
    dd 8
multiboot2_header_end:

section .text
global start
extern kernel_main

start:
    mov esp, stack_top
    push ebx                 ; Multiboot2 info pointer
    push eax                 ; magic
    call kernel_main
    cli
    hlt
    jmp $

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: