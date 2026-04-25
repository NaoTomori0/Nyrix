// src/kernel/user.cpp
#include "user.h"
#include <stdint.h>

void user_task()
{
    const char *msg = "Hello from Nyrix user mode!\n";
    // Вызываем write (eax=1) – работает, т.к. мы в Ring 0
    __asm__ volatile("int $0x80" : : "a"(1), "b"(msg), "c"(28));

    // Просто выходим из функции – управление вернётся в switch_to_user_mode
}

void switch_to_user_mode()
{
    user_task();
    // После возврата из user_task мы продолжим здесь (в commands.cpp)
}