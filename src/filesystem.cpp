#include "os_project.h"

#include <array>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
constexpr int max_files = 16;
constexpr int block_count = 32;
constexpr int block_size = 32;

struct FileEntry {
    std::string name;
    bool used{};
    int size{};
    std::vector<int> blocks;
};

class SimpleFileSystem {
public:
    SimpleFileSystem()
    {
        format();
    }

    void format()
    {
        directory_.fill(FileEntry{});
        bitmap_.fill(false);
        disk_.fill("");
    }

    void create(const std::string &name)
    {
        if (find_file(name) != -1) {
            std::cout << "创建失败: 文件 " << name << " 已存在。\n";
            return;
        }

        const int entry = find_free_entry();
        if (entry == -1) {
            std::cout << "创建失败: 目录项已满。\n";
            return;
        }

        directory_[entry] = FileEntry{name, true, 0, {}};
        std::cout << "创建文件: " << name << '\n';
    }

    void write(const std::string &name, const std::string &content)
    {
        const int idx = find_file(name);
        if (idx == -1) {
            std::cout << "写入失败: 文件 " << name << " 不存在。\n";
            return;
        }

        for (int block : directory_[idx].blocks) {
            bitmap_[block] = false;
            disk_[block].clear();
        }
        directory_[idx].blocks.clear();
        directory_[idx].size = static_cast<int>(content.size());

        for (std::size_t offset = 0; offset < content.size(); offset += block_size) {
            const int block = allocate_block();
            if (block == -1) {
                std::cout << "写入失败: 磁盘空间不足，文件被截断。\n";
                break;
            }
            directory_[idx].blocks.push_back(block);
            disk_[block] = content.substr(offset, block_size);
        }

        std::cout << "写入文件 " << name << ": " << directory_[idx].size
                  << " 字节，占用 " << directory_[idx].blocks.size() << " 个块\n";
    }

    void read(const std::string &name) const
    {
        const int idx = find_file(name);
        if (idx == -1) {
            std::cout << "读取失败: 文件 " << name << " 不存在。\n";
            return;
        }

        std::cout << "读取 " << name << ": ";
        for (int block : directory_[idx].blocks) {
            std::cout << disk_[block];
        }
        std::cout << '\n';
    }

    void remove(const std::string &name)
    {
        const int idx = find_file(name);
        if (idx == -1) {
            std::cout << "删除失败: 文件 " << name << " 不存在。\n";
            return;
        }

        for (int block : directory_[idx].blocks) {
            bitmap_[block] = false;
            disk_[block].clear();
        }
        std::cout << "删除文件: " << name << "，回收 " << directory_[idx].blocks.size() << " 个磁盘块\n";
        directory_[idx] = FileEntry{};
    }

    void list() const
    {
        std::cout << "\n目录:\n文件名\t大小\t块号\n";
        for (const auto &entry : directory_) {
            if (!entry.used) {
                continue;
            }
            std::cout << entry.name << '\t' << entry.size << '\t';
            for (int block : entry.blocks) {
                std::cout << block << ' ';
            }
            std::cout << '\n';
        }
    }

    void print_bitmap() const
    {
        std::cout << "\n空闲空间位示图(0=空闲, 1=占用):\n";
        for (int i = 0; i < block_count; ++i) {
            std::cout << (bitmap_[i] ? 1 : 0);
            if ((i + 1) % 8 == 0) {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }

private:
    int find_file(const std::string &name) const
    {
        for (int i = 0; i < max_files; ++i) {
            if (directory_[i].used && directory_[i].name == name) {
                return i;
            }
        }
        return -1;
    }

    int find_free_entry() const
    {
        for (int i = 0; i < max_files; ++i) {
            if (!directory_[i].used) {
                return i;
            }
        }
        return -1;
    }

    int allocate_block()
    {
        for (int i = 0; i < block_count; ++i) {
            if (!bitmap_[i]) {
                bitmap_[i] = true;
                return i;
            }
        }
        return -1;
    }

    std::array<FileEntry, max_files> directory_{};
    std::array<bool, block_count> bitmap_{};
    std::array<std::string, block_count> disk_{};
};

void run_demo()
{
    SimpleFileSystem fs;
    std::cout << "\n[简易文件系统演示]\n";
    fs.create("hello.txt");
    fs.write("hello.txt", "hello operating system file module");
    fs.read("hello.txt");
    fs.create("note.txt");
    fs.write("note.txt", "bitmap manages free disk blocks");
    fs.list();
    fs.print_bitmap();
    fs.remove("hello.txt");
    fs.list();
    fs.print_bitmap();
}

void run_interactive()
{
    SimpleFileSystem fs;
    std::string name;
    std::string content;

    while (true) {
        std::cout << "\n文件系统操作: 1创建 2写入 3读取 4删除 5目录 6位示图 0返回\n";
        const int op = read_int("请选择: ", 0, 6);
        if (op == 0) {
            return;
        }

        if (op >= 1 && op <= 4) {
            std::cout << "文件名: ";
            std::getline(std::cin, name);
        }

        switch (op) {
        case 1:
            fs.create(name);
            break;
        case 2:
            std::cout << "内容: ";
            std::getline(std::cin, content);
            fs.write(name, content);
            break;
        case 3:
            fs.read(name);
            break;
        case 4:
            fs.remove(name);
            break;
        case 5:
            fs.list();
            break;
        case 6:
            fs.print_bitmap();
            break;
        default:
            break;
        }
    }
}
}

void run_filesystem_module()
{
    std::cout << "\n1. 运行默认演示\n2. 进入交互式文件系统\n";
    const int mode = read_int("请选择: ", 1, 2);
    if (mode == 1) {
        run_demo();
        std::cout << "\n文件系统说明: 该模块使用目录表记录文件元数据，用位示图管理空闲磁盘块。\n"
                  << "写文件时按块分配，删除文件时回收对应块，从而模拟基本文件创建、读写、删除和空闲空间管理。\n";
        wait_for_enter();
    } else {
        run_interactive();
    }
}
