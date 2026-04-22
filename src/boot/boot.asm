; src/boot/boot.asm
; Формат вывода: 32-битный elf (i386)
bits 32

; Мультизагрузочные константы
MAGIC    equ 0x1BADB002      ; Магическое число
FLAGS    equ 0x00000003      ; Выравнивание модулей + информация о памяти
CHECKSUM equ -(MAGIC + FLAGS)

; Секция .multiboot должна быть первой в файле
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Секция .text содержит код
section .text
global start
extern kernel_main          ; Функция ядра на C++

start:
    ; Установка стека (растёт вниз от 0x00200000)
    mov esp, stack_top

    ; Передача управления C++ ядру
    ; В EAX лежит магическое число Multiboot (можно передать как аргумент)
    push eax
    call kernel_main

    ; Если ядро вернулось, зависаем
    cli
    hlt
    jmp $

; Секция .bss для неинициализированных данных (стек)
section .bss
align 16
stack_bottom:
    resb 16384            ; 16 КБ для стека
stack_top:
