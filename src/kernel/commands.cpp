// src/kernel/commands.cpp
#include <stdint.h>
#include <stddef.h>
#include "idt.h" // для outb

extern void terminal_write(const char *str);
extern void terminal_clear();
extern void terminal_setcolor(uint8_t color);
extern size_t pmm_free_pages_count();

static void reboot()
{
    outb(0x64, 0xFE);
    while (1)
        __asm__ volatile("hlt");
}

// Простая функция для преобразования числа в строку (результат в buf, возвращает длину)
static int uint_to_str(size_t num, char *buf, size_t buf_size)
{
    int pos = buf_size - 1;
    buf[pos] = '\0';
    if (num == 0)
    {
        buf[--pos] = '0';
    }
    else
    {
        while (num > 0)
        {
            buf[--pos] = '0' + (num % 10);
            num /= 10;
        }
    }
    return pos; // начало строки
}

void process_command(const char *cmd)
{
    const uint8_t COLOR_OK = 0x0A;
    const uint8_t COLOR_ERR = 0x0C;
    const uint8_t COLOR_WHITE = 0x0F;

    if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0')
    {
        terminal_setcolor(COLOR_OK);
        terminal_write("Available commands:\n");
        terminal_write("  help    - show this message\n");
        terminal_write("  clear   - clear the screen\n");
        terminal_write("  echo    - print text\n");
        terminal_write("  memstat - show memory info\n");
        terminal_write("  ps      - show tasks (stub)\n");
        terminal_write("  reboot  - reboot the system\n");
        terminal_setcolor(COLOR_WHITE);
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
    else if (cmd[0] == 'm' && cmd[1] == 'e' && cmd[2] == 'm' && cmd[3] == 's' && cmd[4] == 't' && cmd[5] == 'a' && cmd[6] == 't' && cmd[7] == '\0')
    {
        size_t free_pages = pmm_free_pages_count();
        terminal_write("Free physical memory: ");
        char buf[20];
        int start = uint_to_str(free_pages, buf, sizeof(buf));
        terminal_write(&buf[start]);
        terminal_write(" pages (");
        // 1 страница = 4 КБ, 1 МБ = 256 страниц (4096 / (1024*1024) = 1/256)
        size_t free_mb = free_pages / 256;
        start = uint_to_str(free_mb, buf, sizeof(buf));
        terminal_write(&buf[start]);
        terminal_write(" MB)\n");
    }
    else if (cmd[0] == 'p' && cmd[1] == 's' && cmd[2] == '\0')
    {
        terminal_write("PID  State\n");
        terminal_write("  0  running (kernel main)\n");
        terminal_write("  1  running (test thread)\n");
    }
    else if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'b' && cmd[3] == 'o' && cmd[4] == 'o' && cmd[5] == 't' && cmd[6] == '\0')
    {
        terminal_setcolor(COLOR_ERR);
        terminal_write("Rebooting...\n");
        terminal_setcolor(COLOR_WHITE);
        reboot();
    }
    else if (cmd[0] != '\0')
    {
        terminal_setcolor(COLOR_ERR);
        terminal_write("Unknown command: ");
        terminal_write(cmd);
        terminal_write("\n");
        terminal_setcolor(COLOR_WHITE);
    }
}