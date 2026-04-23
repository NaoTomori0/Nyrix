// src/kernel/commands.cpp
#include <stdint.h>
#include <stddef.h>
#include "task.h"

extern void terminal_write(const char *str);
extern void terminal_clear();
extern void switch_to_user_mode();

void process_command(const char *cmd)
{
    if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0')
    {
        terminal_write("Available commands: help, clear, echo\n");
    }
    else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r' && cmd[5] == '\0')
    {
        terminal_clear();
    }
    else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ')
    {
        terminal_write(&cmd[5]);
        terminal_write("\n");
    }
    else if (cmd[0] != '\0')
    {
        terminal_write("Unknown command: ");
        terminal_write(cmd);
        terminal_write("\n");
    }
}