#include "os_project.h"

#include <iostream>
#include <limits>

int read_int(const std::string &prompt, int min_value, int max_value)
{
    int value = 0;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value && value >= min_value && value <= max_value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }

        std::cout << "输入无效，请输入 [" << min_value << ", " << max_value << "] 范围内的整数。\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void wait_for_enter()
{
    std::cout << "\n按 Enter 返回主菜单...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static void print_menu()
{
    std::cout << "\n========== 操作系统课程设计 ==========\n"
              << "1. 处理机调度算法模拟\n"
              << "2. 内存管理模拟\n"
              << "3. 进程同步与并发控制\n"
              << "4. 简易文件系统\n"
              << "5. 扩展提升：调度性能测试与优化分析\n"
              << "0. 退出\n";
}

int main()
{
    while (true) {
        print_menu();
        const int choice = read_int("请选择模块: ", 0, 5);

        switch (choice) {
        case 1:
            run_scheduler_module();
            break;
        case 2:
            run_memory_module();
            break;
        case 3:
            run_sync_module();
            break;
        case 4:
            run_filesystem_module();
            break;
        case 5:
            run_extension_module();
            break;
        case 0:
            std::cout << "已退出。\n";
            return 0;
        default:
            break;
        }
    }
}
