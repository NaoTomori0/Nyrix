#include "task.h"
#include "pmm.h"

extern uint8_t terminal_color;
extern void terminal_putchar(char c);
extern "C" void switch_to_task(TaskContext *old_ctx, TaskContext *new_ctx);
extern uint32_t *page_directory;

static uint8_t test_stack[4096] __attribute__((aligned(16)));

static Task main_task;
static Task test_task;
Task *current_task = &main_task;

void test_thread()
{
    uint8_t colors[] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    int idx = 0;
    while (1)
    {
        terminal_color = colors[idx % 6];
        terminal_putchar('T');
        for (volatile int i = 0; i < 5000000; i++)
            ;
        idx++;
    }
}

void tasking_init()
{
    main_task.active = true;
    main_task.stack = nullptr;
    main_task.pgd = (uint32_t)page_directory;
    main_task.is_user = false;
    main_task.parent = nullptr;
    current_task = &main_task;

    test_task.active = true;
    test_task.stack = test_stack;
    test_task.pgd = create_address_space();
    test_task.is_user = false; // фоновый поток работает в Ring 0
    test_task.parent = nullptr;

    uint32_t *sp = (uint32_t *)(test_stack + sizeof(test_stack) - 16);
    *(--sp) = 0x202;
    *(--sp) = 0x08;
    *(--sp) = (uint32_t)&test_thread;
    sp -= 8;
    test_task.ctx.esp = (uint32_t)sp;
    test_task.ctx.ebp = 0;
}

void task_switch()
{
    if (!test_task.active)
        return;
    Task *old = current_task;
    current_task = (current_task == &main_task) ? &test_task : &main_task;
    __asm__ volatile("mov %0, %%cr3" : : "r"(current_task->pgd));
    switch_to_task(&old->ctx, &current_task->ctx);
}

void task_disable_test() { test_task.active = false; }
void task_enable_test() { test_task.active = true; }

// Запуск пользовательской задачи из кода ядра
void start_user_task(Task *task)
{
    // Устанавливаем пользовательские сегменты данных
    __asm__ volatile("mov %0, %%ds" : : "r"(0x23));
    __asm__ volatile("mov %0, %%es" : : "r"(0x23));
    __asm__ volatile("mov %0, %%fs" : : "r"(0x23));
    __asm__ volatile("mov %0, %%gs" : : "r"(0x23));

    // Загружаем сохранённый ESP и выполняем iret
    uint32_t esp = task->ctx.esp;
    __asm__ volatile(
        "mov %0, %%esp\n"
        "iret\n"
        :
        : "r"(esp));
}

void task_exit()
{
    if (!current_task->is_user || !current_task->parent)
    {
        // Невалидное состояние – зависаем
        while (1)
            __asm__("hlt");
    }

    Task *parent = current_task->parent;

    // Освобождаем стек и Page Directory пользовательской задачи
    if (current_task->stack)
    {
        pmm_free_page(current_task->stack);
    }
    if (current_task->pgd && current_task->pgd != (uint32_t)page_directory)
    {
        pmm_free_page((void *)current_task->pgd);
    }

    // Переключаемся на родительскую задачу
    current_task = parent;
    __asm__ volatile("mov %0, %%cr3" : : "r"(parent->pgd));
    switch_to_task(nullptr, &parent->ctx); // nullptr – старый контекст не сохраняем
}