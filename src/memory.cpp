#include "os_project.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct Block {
    int start{};
    int size{};
    bool free{true};
    std::string name;
};

void print_blocks(const std::vector<Block> &blocks)
{
    std::cout << "分区状态:\n序号\t起址\t大小\t状态\t作业\n";
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        std::cout << i << '\t' << blocks[i].start << '\t' << blocks[i].size << '\t'
                  << (blocks[i].free ? "空闲" : "占用") << '\t'
                  << (blocks[i].free ? "-" : blocks[i].name) << '\n';
    }
}

void merge_free(std::vector<Block> &blocks)
{
    for (std::size_t i = 0; i + 1 < blocks.size();) {
        if (blocks[i].free && blocks[i + 1].free) {
            blocks[i].size += blocks[i + 1].size;
            blocks.erase(blocks.begin() + static_cast<long long>(i + 1));
        } else {
            ++i;
        }
    }
}

void allocate(std::vector<Block> &blocks, const std::string &name, int size, bool best_fit)
{
    int chosen = -1;
    for (int i = 0; i < static_cast<int>(blocks.size()); ++i) {
        if (blocks[i].free && blocks[i].size >= size) {
            if (chosen == -1 || !best_fit || blocks[i].size < blocks[chosen].size) {
                chosen = i;
            }
            if (!best_fit) {
                break;
            }
        }
    }

    std::cout << "\n申请 " << name << ": " << size << " KB\n";
    if (chosen == -1) {
        std::cout << "分配失败: 没有足够连续空闲空间。\n";
        return;
    }

    if (blocks[chosen].size > size) {
        blocks.insert(blocks.begin() + chosen + 1,
                      Block{blocks[chosen].start + size, blocks[chosen].size - size, true, ""});
    }
    blocks[chosen].size = size;
    blocks[chosen].free = false;
    blocks[chosen].name = name;
    print_blocks(blocks);
}

void release(std::vector<Block> &blocks, const std::string &name)
{
    std::cout << "\n释放 " << name << '\n';
    auto it = std::find_if(blocks.begin(), blocks.end(), [&](const Block &block) {
        return !block.free && block.name == name;
    });

    if (it == blocks.end()) {
        std::cout << "释放失败: 未找到该作业。\n";
        return;
    }

    it->free = true;
    it->name.clear();
    merge_free(blocks);
    print_blocks(blocks);
}

void run_partition_demo(bool best_fit)
{
    std::vector<Block> blocks{{0, 640, true, ""}};

    std::cout << "\n[" << (best_fit ? "最佳适应 BF" : "首次适应 FF") << "] 动态分区管理演示\n";
    print_blocks(blocks);
    allocate(blocks, "A", 130, best_fit);
    allocate(blocks, "B", 60, best_fit);
    allocate(blocks, "C", 100, best_fit);
    release(blocks, "B");
    allocate(blocks, "D", 50, best_fit);
    release(blocks, "A");
    allocate(blocks, "E", 120, best_fit);
}

int find_page(const std::vector<int> &frames, int page)
{
    auto it = std::find(frames.begin(), frames.end(), page);
    return it == frames.end() ? -1 : static_cast<int>(it - frames.begin());
}

void print_frames(const std::vector<int> &frames)
{
    std::cout << "内存块: ";
    for (int page : frames) {
        if (page == -1) {
            std::cout << "[ ] ";
        } else {
            std::cout << '[' << page << "] ";
        }
    }
    std::cout << '\n';
}

void run_fifo(const std::vector<int> &pages, int frame_count)
{
    std::vector<int> frames(frame_count, -1);
    int next = 0;
    int faults = 0;

    std::cout << "\n[FIFO 页面置换]\n";
    for (int page : pages) {
        if (find_page(frames, page) == -1) {
            frames[next] = page;
            next = (next + 1) % frame_count;
            ++faults;
            std::cout << "访问 " << page << ": 缺页 -> ";
        } else {
            std::cout << "访问 " << page << ": 命中 -> ";
        }
        print_frames(frames);
    }
    std::cout << std::fixed << std::setprecision(2)
              << "缺页次数: " << faults << ", 缺页率: " << faults * 100.0 / pages.size() << "%\n";
}

void run_lru(const std::vector<int> &pages, int frame_count)
{
    std::vector<int> frames(frame_count, -1);
    std::vector<int> last_used(frame_count, -1);
    int faults = 0;

    std::cout << "\n[LRU 页面置换]\n";
    for (int time = 0; time < static_cast<int>(pages.size()); ++time) {
        const int page = pages[time];
        const int pos = find_page(frames, page);
        if (pos != -1) {
            last_used[pos] = time;
            std::cout << "访问 " << page << ": 命中 -> ";
        } else {
            int victim = 0;
            for (int i = 1; i < frame_count; ++i) {
                if (frames[i] == -1 || last_used[i] < last_used[victim]) {
                    victim = i;
                }
            }
            frames[victim] = page;
            last_used[victim] = time;
            ++faults;
            std::cout << "访问 " << page << ": 缺页 -> ";
        }
        print_frames(frames);
    }
    std::cout << std::fixed << std::setprecision(2)
              << "缺页次数: " << faults << ", 缺页率: " << faults * 100.0 / pages.size() << "%\n";
}
}

void run_memory_module()
{
    std::vector<int> pages{7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};

    std::cout << "\n1. 使用默认页面访问串\n2. 手动输入页面访问串\n";
    const int mode = read_int("请选择输入方式: ", 1, 2);
    if (mode == 2) {
        const int count = read_int("页面访问次数(1-64): ", 1, 64);
        pages.clear();
        for (int i = 0; i < count; ++i) {
            pages.push_back(read_int("第 " + std::to_string(i + 1) + " 次访问页号: ", 0, 999));
        }
    }

    const int frames = read_int("物理块数量(1-16): ", 1, 16);
    run_partition_demo(false);
    run_partition_demo(true);
    run_fifo(pages, frames);
    run_lru(pages, frames);

    std::cout << "\n分析说明:\n"
              << "FF 查找速度快，但容易在低地址产生碎片；BF 更节省单次剩余空间，但会留下更多小碎片。\n"
              << "FIFO 实现简单但可能淘汰常用页面；LRU 利用局部性原理，通常缺页率更低。\n";
    wait_for_enter();
}
