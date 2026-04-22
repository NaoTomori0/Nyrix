; src/boot/gdt_flush.asm
bits 32
global gdt_flush

gdt_flush:
    mov eax, [esp+4]    ; параметр - указатель на GDTPtr
    lgdt [eax]          ; загружаем GDT

    ; Обновляем сегментные регистры данных
    mov ax, 0x10        ; селектор 0x10 = запись 2 в GDT (данные)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Дальний прыжок для обновления CS (код)
    jmp 0x08:.flush     ; 0x08 = запись 1 в GDT (код)
.flush:
    ret
