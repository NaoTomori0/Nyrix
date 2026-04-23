; src/boot/interrupts.asm
bits 32

global idt_flush
global isr_stub_table

extern fault_handler
extern irq_handler


; ------------------------------
; Загрузка IDT
; ------------------------------
idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

; ------------------------------
; Макросы для создания ISR/IRQ заглушек
; ------------------------------
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0
    push %1
    jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1
    jmp isr_common
%endmacro

%macro IRQ 2
global isr%2
isr%2:
    push 0
    push %2
    jmp irq_common
%endmacro

; ------------------------------
; Исключения CPU (0-31)
; ------------------------------
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; ------------------------------
; Аппаратные прерывания IRQ 0-15 (вектора 32-47)
; ------------------------------
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47



; ------------------------------
; Общий обработчик исключений
; ------------------------------
isr_common:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call fault_handler
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

; ------------------------------
; Общий обработчик IRQ
; ------------------------------
irq_common:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call irq_handler
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

; ------------------------------
; Таблица указателей на заглушки (первые 48 векторов)
; ------------------------------
section .data
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 48
    dd isr%+i
%assign i i+1
%endrep