; src/boot/task_switch.asm
bits 32
global switch_to_task

; void switch_to_task(TaskContext* old_ctx, TaskContext* new_ctx)
switch_to_task:
    ; 1. Сохраняем old_ctx
    mov eax, [esp+4]            ; old_ctx

    ; Сохраняем eip (адрес возврата, лежит на стеке)
    pop dword [eax+0]           ; забираем адрес возврата, теперь esp указывает на old_ctx
    ; Теперь стек: [old_ctx] [new_ctx] (но мы уже сняли адрес возврата)

    ; Сохраняем eflags
    pushfd
    pop dword [eax+4]

    ; Сохраняем регистры общего назначения
    mov [eax+8],  eax           ; это значение eax будет перезаписано позже, но пока пусть будет
    mov [eax+12], ecx
    mov [eax+16], edx
    mov [eax+20], ebx
    ; esp сейчас указывает на new_ctx, но после pop его значение другое
    ; Мы сохраним esp таким, каким он был до входа в функцию + 4 (потому что мы сняли адрес возврата)
    lea ecx, [esp]              ; текущий esp
    mov [eax+24], ecx
    mov [eax+28], ebp
    mov [eax+32], esi
    mov [eax+36], edi

    ; 2. Загружаем new_ctx
    mov eax, [esp]              ; new_ctx (теперь esp указывает прямо на него)
    ; Восстанавливаем регистры
    push dword [eax+4]          ; eflags
    popfd
    mov esp, [eax+24]
    mov ebp, [eax+28]
    mov esi, [eax+32]
    mov edi, [eax+36]
    mov ebx, [eax+20]
    mov edx, [eax+16]
    mov ecx, [eax+12]
    ; eip кладём на стек
    push dword [eax+0]
    ; восстанавливаем eax
    mov eax, [eax+8]
    ret