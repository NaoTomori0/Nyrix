// src/kernel/commands.cpp
#include <stdint.h>
#include <stddef.h>

extern void terminal_write(const char* str);
extern void terminal_clear();

// Прототипы функций
void process_command(const char* cmd);

void process_command(const char* cmd) {
    // Сравниваем строки (наивное сравнение)
    if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0') {
        terminal_write("Available commands: help, clear, memstat, echo <text>\n");
    } else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r' && cmd[5] == '\0') {
        terminal_clear();
    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
        // Вывести всё после "echo "
        terminal_write(&cmd[5]);
        terminal_write("\n");
    } else if (cmd[0] == 'm' && cmd[1] == 'e' && cmd[2] == 'm' && cmd[3] == 's' && cmd[4] == 't' && cmd[5] == 'a' && cmd[6] == 't' && cmd[7] == '\0') {
        // Заглушка – позже добавим вывод памяти
        terminal_write("Memory status: PMM active, heap active.\n");
    } else if (cmd[0] == '\0') {
        // пустой ввод, ничего не делаем
    } else {
        terminal_write("Unknown command: ");
        terminal_write(cmd);
        terminal_write("\n");
    }
}