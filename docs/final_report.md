# 《操作系统》课程设计实验报告

## 基础实验

本课程设计围绕操作系统的核心机制展开，使用 C++17 完成处理机调度、内存管理、进程同步与并发控制、简易文件系统四个基础实验，并在此基础上扩展实现调度性能测试与优化分析模块。项目采用菜单式结构组织，用户可以通过主菜单选择不同实验模块运行，也可以在部分模块中选择默认样例或手动输入参数。

项目源码主要目录如下：

```text
os_project
├── include/os_project.h
├── src/main.cpp
├── src/scheduler.cpp
├── src/memory.cpp
├── src/sync.cpp
├── src/filesystem.cpp
├── src/extension.cpp
├── docs/report_template.md
├── docs/final_report.md
└── Makefile
```

编译运行方式如下：

```bash
make
./os_course_design
```

如果在 Windows 下使用 WSL，可以执行：

```bash
wsl make
wsl ./os_course_design
```

### 实验1：处理机调度算法模拟

#### 1.1 实验目的

处理机调度是操作系统进行资源分配和任务管理的核心功能之一。本实验通过模拟多种经典调度算法，理解进程到达时间、服务时间、优先级、时间片等参数对系统运行结果的影响，并统计不同算法下的等待时间、周转时间等性能指标。

本实验实现的调度算法包括：

- 先来先服务调度算法 FCFS。
- 短作业优先调度算法 SJF，采用非抢占式实现。
- 时间片轮转调度算法 RR，支持用户输入时间片大小。
- 优先级调度算法 Priority，采用非抢占式实现，数值越小优先级越高。

#### 1.2 实验设计

调度模块位于 `src/scheduler.cpp`。程序使用 `Process` 结构体表示进程控制块 PCB 的核心信息，包括进程号、到达时间、服务时间、优先级、剩余运行时间、开始时间、完成时间和完成状态。

关键数据结构如下：

```cpp
struct Process {
    int pid{};
    int arrival{};
    int burst{};
    int priority{};
    int remaining{};
    int start{-1};
    int finish{};
    bool completed{};
};
```

其中，`arrival` 表示进程到达时间，`burst` 表示需要 CPU 服务的时间，`priority` 表示进程优先级，`remaining` 用于 RR 算法记录剩余时间，`start` 和 `finish` 用于计算响应时间、等待时间和周转时间。

程序提供两种输入方式：默认样例和手动输入。默认样例便于快速演示和截图，手动输入则满足课程要求中“支持动态输入进程参数”的要求。

输入处理逻辑如下：

```cpp
std::vector<Process> load_processes()
{
    std::cout << "\n1. 使用默认样例\n2. 手动输入进程参数\n";
    const int mode = read_int("请选择输入方式: ", 1, 2);

    if (mode == 1) {
        return {
            {1, 0, 7, 2},
            {2, 2, 4, 1},
            {3, 4, 1, 4},
            {4, 5, 4, 3},
            {5, 6, 2, 2},
        };
    }

    const int n = read_int("进程数量(1-32): ", 1, 32);
    std::vector<Process> processes;
    for (int i = 0; i < n; ++i) {
        Process p;
        p.pid = i + 1;
        p.arrival = read_int("到达时间: ", 0, 10000);
        p.burst = read_int("服务时间: ", 1, 10000);
        p.priority = read_int("优先级(数字越小越高): ", 1, 10000);
        processes.push_back(p);
    }
    return processes;
}
```

#### 1.3 算法实现

FCFS 算法每次选择当前已经到达且到达时间最早的进程运行。该算法实现简单，调度开销低，但若长作业先到，会导致后续短作业长时间等待。

核心逻辑如下：

```cpp
if (!p.completed && p.arrival <= time &&
    (chosen == -1 || p.arrival < processes[chosen].arrival ||
     (p.arrival == processes[chosen].arrival && p.pid < processes[chosen].pid))) {
    chosen = i;
}
```

SJF 算法每次在已经到达的进程中选择服务时间最短的进程运行。它通常能够降低平均等待时间，但需要提前知道服务时间，且长作业可能因为短作业不断到来而等待较久。

核心逻辑如下：

```cpp
if (!p.completed && p.arrival <= time &&
    (chosen == -1 || p.burst < processes[chosen].burst ||
     (p.burst == processes[chosen].burst && p.arrival < processes[chosen].arrival))) {
    chosen = i;
}
```

RR 算法使用就绪队列保存等待运行的进程，每次最多运行一个时间片。若进程未运行完成，则重新进入就绪队列尾部。该算法适合分时系统，可以提高交互响应性。

核心逻辑如下：

```cpp
const int slice = std::min(processes[idx].remaining, quantum);
std::cout << "P" << processes[idx].pid << "(" << time << "-" << time + slice << ") ";
time += slice;
processes[idx].remaining -= slice;

if (processes[idx].remaining == 0) {
    processes[idx].finish = time;
    processes[idx].completed = true;
} else {
    ready.push(idx);
    in_queue[idx] = true;
}
```

优先级调度算法在已到达进程中选择优先级最高的进程运行。本项目约定优先级数值越小，优先级越高。

核心逻辑如下：

```cpp
if (!p.completed && p.arrival <= time &&
    (chosen == -1 || p.priority < processes[chosen].priority ||
     (p.priority == processes[chosen].priority && p.arrival < processes[chosen].arrival))) {
    chosen = i;
}
```

#### 1.4 实验结果与分析

程序输出每种算法的运行顺序，并统计每个进程的完成时间、等待时间、周转时间，同时计算平均等待时间和平均周转时间。

等待时间和周转时间计算方式如下：

```text
周转时间 = 完成时间 - 到达时间
等待时间 = 周转时间 - 服务时间
```

默认样例中，进程参数如下：

```text
PID  到达时间  服务时间  优先级
P1   0        7        2
P2   2        4        1
P3   4        1        4
P4   5        4        3
P5   6        2        2
```

在时间片为 2 的情况下，RR 的运行顺序示例为：

```text
P1(0-2) P2(2-4) P1(4-6) P3(6-7) P2(7-9)
P4(9-11) P5(11-13) P1(13-15) P4(15-17) P1(17-18)
```

算法分析如下：

- FCFS 的实现最简单，适合负载较稳定、任务差异较小的场景，但可能出现“护航效应”。
- SJF 在默认样例中通常具有较低的平均等待时间，但它依赖服务时间估计，不适合无法预测运行时间的实际场景。
- RR 的响应性较好，短时间内多个进程都可以获得 CPU，但上下文切换次数较多，时间片设置会明显影响性能。
- 优先级调度能够体现任务重要程度，但如果不加入老化机制，低优先级进程可能长期等待。

#### 1.5 与课程要求对应情况

- 支持动态输入进程参数：已实现，可手动输入进程数量、到达时间、服务时间和优先级。
- 输出进程运行顺序：已实现，FCFS/SJF/Priority 输出进程顺序，RR 输出运行时间段。
- 输出周转时间等结果：已实现，输出等待时间、周转时间、平均等待时间和平均周转时间。
- 分析不同调度算法特点与性能差异：已在程序输出和报告中分析。

### 实验2：内存管理模拟

#### 2.1 实验目的

内存管理是操作系统资源管理的重要部分。本实验通过动态分区分配与页面置换算法模拟，理解连续内存分配、内存回收、碎片合并、页面命中、缺页中断和虚拟存储局部性原理。

本实验实现内容包括：

- 动态分区管理：首次适应算法 FF。
- 动态分区管理：最佳适应算法 BF。
- 页面置换算法：FIFO。
- 页面置换算法：LRU。

#### 2.2 动态分区管理设计

动态分区模块使用 `Block` 结构体表示一段连续内存分区。每个分区记录起始地址、大小、是否空闲和所属作业名。

关键数据结构如下：

```cpp
struct Block {
    int start{};
    int size{};
    bool free{true};
    std::string name;
};
```

首次适应 FF 从低地址开始查找第一个能够满足申请大小的空闲分区。最佳适应 BF 则查找能够满足申请且剩余空间最小的空闲分区。

分配逻辑如下：

```cpp
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
```

如果被选中的空闲分区大于申请大小，则将其切分为“已分配分区”和“剩余空闲分区”。

```cpp
if (blocks[chosen].size > size) {
    blocks.insert(blocks.begin() + chosen + 1,
                  Block{blocks[chosen].start + size,
                        blocks[chosen].size - size,
                        true,
                        ""});
}
blocks[chosen].size = size;
blocks[chosen].free = false;
blocks[chosen].name = name;
```

释放内存时，程序会查找对应作业名，将该分区标记为空闲，并调用 `merge_free` 合并相邻空闲分区，从而减少外部碎片。

```cpp
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
```

#### 2.3 页面置换设计

页面置换模块使用页面访问串和物理块数组进行模拟。用户可以选择默认页面访问串，也可以手动输入访问序列和物理块数量。

默认页面访问串如下：

```text
7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2
```

FIFO 页面置换算法按照页面进入内存的先后顺序淘汰最早进入的页面。

```cpp
if (find_page(frames, page) == -1) {
    frames[next] = page;
    next = (next + 1) % frame_count;
    ++faults;
}
```

LRU 页面置换算法记录每个物理块最近一次被访问的时间，缺页时淘汰最久未使用的页面。

```cpp
if (pos != -1) {
    last_used[pos] = time;
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
}
```

缺页率计算方式如下：

```text
缺页率 = 缺页次数 / 页面访问总次数 * 100%
```

#### 2.4 实验结果与分析

动态分区管理默认以 640 KB 内存为初始空闲分区，依次执行申请和释放操作：

```text
申请 A: 130 KB
申请 B: 60 KB
申请 C: 100 KB
释放 B
申请 D: 50 KB
释放 A
申请 E: 120 KB
```

该过程能够展示分区切分、空闲分区形成、内存回收以及空闲区合并。FF 和 BF 在简单场景中可能得到相似结果，但在复杂分配序列中，FF 倾向于低地址快速分配，BF 倾向于选择最贴合大小的空闲区。

页面置换实验中，在物理块数量为 3 时，FIFO 和 LRU 会输出每次访问后的物理块状态，并统计缺页次数与缺页率。一般情况下，LRU 利用了程序访问的局部性原理，能够减少对近期频繁访问页面的淘汰，因此表现通常比 FIFO 更稳定。

#### 2.5 与课程要求对应情况

- 展示内存分配与回收过程：已实现，动态分区模块逐步输出分区状态。
- 实现页面置换机制：已实现 FIFO 和 LRU。
- 统计缺页次数与缺页率：已实现。
- 理解分页、分区与虚拟存储思想：报告中结合分区分配、页面置换和局部性原理进行分析。

### 实验3：进程同步与并发控制

#### 3.1 实验目的

进程同步与并发控制是操作系统解决并发执行中数据竞争、互斥访问、同步等待和死锁问题的重要机制。本实验使用 C++ 标准线程库模拟典型同步问题，掌握互斥锁、条件变量和多线程协作方式。

本实验实现内容包括：

- 生产者—消费者问题。
- 读者—写者问题。
- 哲学家进餐问题。

#### 3.2 生产者—消费者问题

生产者—消费者问题模拟多个执行流共享有限缓冲区的场景。生产者向缓冲区写入数据，消费者从缓冲区取出数据。若缓冲区满，生产者应等待；若缓冲区空，消费者应等待。

本项目使用如下同步对象：

- `std::mutex mutex`：保护缓冲区、读写位置和计数器。
- `std::condition_variable not_full`：表示缓冲区未满。
- `std::condition_variable not_empty`：表示缓冲区非空。
- `std::mutex output_mutex`：保护控制台输出，避免多线程输出交错。

关键代码如下：

```cpp
not_full.wait(lock, [&] { return count < buffer_size; });
buffer[in_pos] = item;
in_pos = (in_pos + 1) % buffer_size;
++count;
lock.unlock();
not_empty.notify_one();
```

消费者对应逻辑如下：

```cpp
not_empty.wait(lock, [&] { return count > 0; });
const int item = buffer[out_pos];
out_pos = (out_pos + 1) % buffer_size;
--count;
lock.unlock();
not_full.notify_one();
```

这种设计保证了缓冲区读写不会越界，也不会出现生产者覆盖未消费数据或消费者读取无效数据的问题。

#### 3.3 读者—写者问题

读者—写者问题模拟共享数据读多写少的场景。多个读者可以并发读取数据，但写者必须独占访问共享数据。

本项目使用 `read_count` 记录当前读者数量。当第一个读者进入时锁住写锁，当最后一个读者离开时释放写锁。这样可以允许多个读者并行读取，同时保证写者写入时没有读者并发访问。

关键代码如下：

```cpp
{
    std::lock_guard<std::mutex> count_lock(read_count_mutex);
    ++read_count;
    if (read_count == 1) {
        rw_mutex.lock();
    }
}

// 读取 shared_data

{
    std::lock_guard<std::mutex> count_lock(read_count_mutex);
    --read_count;
    if (read_count == 0) {
        rw_mutex.unlock();
    }
}
```

写者通过 `rw_mutex` 获得独占访问权：

```cpp
std::lock_guard<std::mutex> write_lock(rw_mutex);
shared_data += 10;
```

#### 3.4 哲学家进餐问题

哲学家进餐问题用于说明死锁产生的条件。五个哲学家围坐一圈，每人左右各有一把筷子，只有同时拿到两把筷子才能进餐。如果所有哲学家都先拿左边筷子再等待右边筷子，就可能产生循环等待并导致死锁。

本项目使用 `std::lock` 同时申请两把互斥锁，避免多个线程在锁获取顺序上形成死锁。

关键代码如下：

```cpp
std::lock(forks[first], forks[second]);
std::lock_guard<std::mutex> lock_first(forks[first], std::adopt_lock);
std::lock_guard<std::mutex> lock_second(forks[second], std::adopt_lock);
```

`std::lock` 能够以避免死锁的方式同时锁定多个互斥量，`std::adopt_lock` 表示互斥量已经被锁定，由 `lock_guard` 接管释放。

#### 3.5 实验结果与分析

生产者—消费者模块能够看到生产者按顺序生产数据，消费者按缓冲区顺序消费数据，说明条件变量正确控制了空/满状态。

读者—写者模块能够看到多个读者读取相同共享数据，而写者写入时独占共享变量，说明互斥控制有效。

哲学家进餐模块能够看到五个哲学家交替思考和进餐，程序能够正常结束，说明不存在死锁。

#### 3.6 与课程要求对应情况

- 使用多线程模拟并发执行：已实现，使用 `std::thread`。
- 正确使用互斥锁、条件变量等同步机制：已实现。
- 避免死锁与数据竞争：已实现，关键共享变量均有互斥保护，哲学家进餐使用 `std::lock` 避免死锁。
- 覆盖典型同步问题：已实现生产者—消费者、读者—写者、哲学家进餐三个问题。

### 实验4：简易文件系统设计与实现

#### 4.1 实验目的

文件系统负责对外提供文件创建、读写、删除等接口，并在内部管理磁盘空间和文件元数据。本实验通过模拟简易文件系统，理解目录项、文件控制信息、磁盘块分配和空闲空间管理机制。

本实验实现内容包括：

- 文件创建。
- 文件写入。
- 文件读取。
- 文件删除。
- 目录查看。
- 空闲空间位示图管理。

#### 4.2 实验设计

文件系统模块位于 `src/filesystem.cpp`，核心类为 `SimpleFileSystem`。系统使用固定数量的目录项和磁盘块模拟一个小型磁盘。

主要常量如下：

```cpp
constexpr int max_files = 16;
constexpr int block_count = 32;
constexpr int block_size = 32;
```

文件目录项结构如下：

```cpp
struct FileEntry {
    std::string name;
    bool used{};
    int size{};
    std::vector<int> blocks;
};
```

其中，`name` 表示文件名，`used` 表示目录项是否被使用，`size` 表示文件大小，`blocks` 保存文件占用的磁盘块号。

文件系统内部维护三个核心成员：

```cpp
std::array<FileEntry, max_files> directory_{};
std::array<bool, block_count> bitmap_{};
std::array<std::string, block_count> disk_{};
```

`directory_` 用于保存目录信息，`bitmap_` 用于管理空闲空间，`disk_` 用字符串模拟磁盘块中的数据。

#### 4.3 基本操作实现

创建文件时，程序首先检查文件是否已存在，然后查找空闲目录项。

```cpp
void create(const std::string &name)
{
    if (find_file(name) != -1) {
        std::cout << "创建失败: 文件已存在。\n";
        return;
    }

    const int entry = find_free_entry();
    if (entry == -1) {
        std::cout << "创建失败: 目录项已满。\n";
        return;
    }

    directory_[entry] = FileEntry{name, true, 0, {}};
}
```

写入文件时，程序先回收原来占用的块，再按块大小重新分配磁盘块。这样可以模拟覆盖写入和空间重新分配过程。

```cpp
for (int block : directory_[idx].blocks) {
    bitmap_[block] = false;
    disk_[block].clear();
}
directory_[idx].blocks.clear();
directory_[idx].size = static_cast<int>(content.size());
```

磁盘块分配使用位示图，从低号块开始查找空闲块。

```cpp
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
```

读取文件时，根据目录项中的块号顺序读取对应磁盘块内容。

```cpp
for (int block : directory_[idx].blocks) {
    std::cout << disk_[block];
}
```

删除文件时，回收该文件占用的所有磁盘块，并清空目录项。

```cpp
for (int block : directory_[idx].blocks) {
    bitmap_[block] = false;
    disk_[block].clear();
}
directory_[idx] = FileEntry{};
```

#### 4.4 实验结果与分析

默认演示过程如下：

```text
创建 hello.txt
写入 hello.txt
读取 hello.txt
创建 note.txt
写入 note.txt
显示目录
显示空闲空间位示图
删除 hello.txt
再次显示目录与位示图
```

演示结果能够看到文件写入后对应磁盘块从 0 变为 1，删除文件后相应块重新变为空闲。该过程体现了文件系统中目录管理、数据块分配和空闲空间回收的基本思想。

本实验采用连续逻辑的“块号列表”方式记录文件占用块，虽然不是完整真实文件系统，但能够清晰展示文件元数据与存储块之间的映射关系。

#### 4.5 与课程要求对应情况

- 支持文件创建、读写、删除基本操作：已实现。
- 实现空闲空间管理：已实现位示图 `bitmap_`。
- 理解文件组织与存储管理机制：已通过目录项、磁盘块、块号列表和位示图模拟。
- 支持交互式演示：已实现默认演示和交互式文件系统两种模式。

## 扩展实验（Optional）

### 实验5：调度性能测试与 HRRN 优化分析实验

#### 5.1 选题动机与创新点

在完成基础实验后，本项目选择“调度与性能优化”作为扩展方向。相比 Linux 内核源码编译、内核模块开发或设备驱动实验，该方向具有更高的可行性和可展示性：不依赖特殊内核版本、硬件平台或 root 权限，能够直接在当前项目中运行，并生成可用于报告的实验数据。

本扩展实验的主要动机如下：

- 基础调度实验已经实现 FCFS、SJF、RR、Priority 等算法，扩展模块可以直接复用处理机调度知识。
- 课程要求鼓励调度算法改进、系统性能测试与分析，本实验正好对应“调度与性能优化”方向。
- 单纯运行一个调度算法只能观察局部结果，扩展模块通过统一负载、统一指标、统一输出表格进行对比，更符合工程实践中的性能评测思路。
- 增加 HRRN 最高响应比优先算法，体现对基础算法的改进和探索。

本实验的创新点主要体现在：

- 自动构造两类测试负载：balanced 均衡负载和 bursty 突发负载。
- 同时比较五种算法：FCFS、SJF、RR、Priority、HRRN。
- 增加平均响应时间、上下文切换次数、吞吐率等指标，不只统计等待时间和周转时间。
- 自动导出 `docs/scheduler_benchmark.csv`，便于在报告中整理成表格。
- 使用 HRRN 缓解 SJF 可能导致长作业饥饿的问题。

#### 5.2 实验设计与实现

扩展模块位于 `src/extension.cpp`。其基本流程如下：

```text
构造测试负载
输入 RR 时间片
分别运行 FCFS、SJF、RR、Priority、HRRN
统计平均等待时间、平均周转时间、平均响应时间
统计上下文切换次数、完成时长、吞吐率
输出控制台表格
生成 CSV 文件
输出实验结论
```

扩展模块使用与基础调度模块类似的 `Process` 结构体，同时增加 `Metrics` 结构体保存性能指标。

```cpp
struct Metrics {
    std::string workload;
    std::string algorithm;
    double average_waiting{};
    double average_turnaround{};
    double average_response{};
    int context_switches{};
    int makespan{};
    double throughput{};
};
```

本实验构造了两组进程负载。balanced 负载中进程服务时间相对均衡，适合观察一般调度场景；bursty 负载中同时包含很长作业和很短作业，适合观察不同算法面对突发短作业时的表现。

balanced 负载如下：

```cpp
return {
    {1, 0, 6, 3}, {2, 1, 2, 1}, {3, 2, 8, 4}, {4, 3, 3, 2},
    {5, 4, 4, 3}, {6, 6, 5, 2}, {7, 8, 2, 5}, {8, 9, 7, 1},
};
```

bursty 负载如下：

```cpp
return {
    {1, 0, 12, 4}, {2, 1, 1, 1}, {3, 2, 2, 2}, {4, 3, 1, 3},
    {5, 5, 9, 5},  {6, 6, 2, 1}, {7, 7, 1, 2}, {8, 9, 8, 4},
};
```

HRRN 算法的核心思想是计算每个已到达进程的响应比：

```text
响应比 = (等待时间 + 服务时间) / 服务时间
```

响应比越高，越优先调度。代码实现如下：

```cpp
const double wait_i = time - processes[i].arrival;
const double wait_chosen = time - processes[chosen].arrival;
const double ratio_i = (wait_i + processes[i].burst) / processes[i].burst;
const double ratio_chosen = (wait_chosen + processes[chosen].burst) / processes[chosen].burst;
better = ratio_i > ratio_chosen;
```

性能指标计算如下：

```cpp
const int turnaround = p.finish - p.arrival;
const int waiting = turnaround - p.burst;
const int response = p.start - p.arrival;
```

吞吐率计算如下：

```cpp
throughput = processes.size() / static_cast<double>(makespan);
```

CSV 导出逻辑如下：

```cpp
std::ofstream out("docs/scheduler_benchmark.csv");
out << "workload,algorithm,average_waiting,average_turnaround,"
       "average_response,context_switches,makespan,throughput\n";
for (const auto &m : results) {
    out << m.workload << ',' << m.algorithm << ',' << m.average_waiting << ','
        << m.average_turnaround << ',' << m.average_response << ','
        << m.context_switches << ',' << m.makespan << ',' << m.throughput << '\n';
}
```

#### 5.3 实验结果与分析

使用 RR 时间片 `q=3` 运行扩展模块，得到如下结果：

```text
负载       算法       平均等待  平均周转  平均响应  切换次数  完成时长  吞吐率
balanced  FCFS       12.12     16.75     12.12     7        37       0.22
balanced  SJF        9.00      13.62     9.00      7        37       0.22
balanced  RR(q=3)    14.00     18.62     7.38      13       37       0.22
balanced  Priority   11.88     16.50     11.88     7        37       0.22
balanced  HRRN       9.50      14.12     9.50      7        37       0.22
bursty    FCFS       12.88     17.38     12.88     7        36       0.22
bursty    SJF        10.50     15.00     10.50     7        36       0.22
bursty    RR(q=3)    10.00     14.50     4.25      14       36       0.22
bursty    Priority   11.00     15.50     11.00     7        36       0.22
bursty    HRRN       10.62     15.12     10.62     7        36       0.22
```

分析如下：

- 在 balanced 负载中，SJF 的平均等待时间最低，为 9.00，说明当任务长度差异较明显时，优先执行短作业可以降低整体等待时间。
- HRRN 在 balanced 负载中的平均等待时间为 9.50，略高于 SJF，但它会考虑等待时间，因此公平性更好。
- RR 在 balanced 负载中平均响应时间为 7.38，低于 FCFS、Priority 和 HRRN，说明时间片轮转能让进程更早获得 CPU。
- RR 的上下文切换次数为 13 或 14，明显高于非抢占式算法的 7，说明 RR 需要付出更多调度切换开销。
- 在 bursty 负载中，RR 的平均响应时间最低，为 4.25，体现出分时调度对短作业和交互式任务更友好。
- HRRN 通过响应比动态提高等待时间较长进程的优先级，可以缓解 SJF 中长作业等待过久的问题。

扩展模块还会生成 `docs/scheduler_benchmark.csv`，可作为报告附件或实验数据表格来源。

#### 5.4 扩展实验总结

本扩展实验在基础调度算法之上进一步完成了性能评测和算法优化分析。通过构造不同负载并统一统计指标，可以更直观地比较不同调度算法的优势和不足。

本实验说明：

- 没有一种调度算法在所有指标上都绝对最优。
- SJF 在平均等待时间方面表现较好，但可能牺牲公平性。
- RR 在响应时间方面表现较好，但上下文切换次数较多。
- Priority 能体现任务重要性，但需要配合老化机制避免低优先级任务饥饿。
- HRRN 在 SJF 的基础上加入等待时间因素，是一种兼顾效率和公平性的改进算法。

从工程角度看，扩展模块不仅实现了算法，还完成了测试负载构造、指标计算、结果表格输出和 CSV 文件导出，具备较完整的实验闭环。

## 课程学习心得体会与总结

通过本次操作系统课程设计，我对操作系统中“资源管理”和“并发控制”的理解更加具体。课堂学习中，处理机调度、内存管理、同步互斥和文件系统往往是以概念、公式和流程图的形式出现，而在编程实现过程中，需要把这些抽象机制落实为数据结构、状态变量、队列、锁和输出结果。

在处理机调度实验中，我体会到不同调度算法本质上是在不同目标之间做权衡。FCFS 强调简单和先后顺序，SJF 强调整体平均等待时间，RR 强调响应性，Priority 强调任务重要性。扩展实验中的 HRRN 则让我进一步理解到，调度算法不仅要追求效率，也要考虑公平性。

在内存管理实验中，我通过动态分区的分配和回收过程理解了外部碎片产生的原因，也通过 FIFO 和 LRU 页面置换理解了缺页率和局部性原理。尤其是 LRU 算法中记录最近访问时间的做法，让我认识到虚拟存储管理并不是简单地“内存不够就换出”，而是需要根据访问历史预测未来可能使用的页面。

在同步实验中，我对多线程程序的不确定性有了更直观的认识。如果没有互斥锁和条件变量，多个线程同时访问共享变量会产生数据竞争；如果锁的申请顺序不合理，又可能造成死锁。生产者—消费者、读者—写者和哲学家进餐三个问题分别体现了同步等待、共享读写和死锁避免，是理解并发控制的典型案例。

在文件系统实验中，我通过目录项、位示图和磁盘块模拟了文件从逻辑名称到物理存储块的映射过程。虽然本项目实现的是简化文件系统，但它覆盖了文件创建、读写、删除、目录管理和空闲空间管理等基本思想，为理解真实文件系统打下了基础。

本项目整体采用 C++17 实现，相比纯过程式代码，使用结构体、类、`std::vector`、`std::array`、`std::thread` 等标准库工具后，程序结构更清晰，也更容易扩展。通过 Makefile 统一编译，也使项目更接近实际工程组织方式。

从课程要求来看，本项目已完成基础必做部分的四类实验，并完成扩展提升部分中的“调度与性能优化”方向。项目不仅能够运行演示，还能输出统计结果、生成 CSV 数据文件，并支持后续继续扩展为图形化展示、更多页面置换算法、更多同步问题或更复杂的文件系统。

最后，本次课程设计让我认识到，操作系统并不是孤立的理论知识，而是一组围绕资源、状态和并发组织起来的工程机制。只有真正写代码模拟这些机制，才能更深入地理解算法背后的取舍和系统运行中的复杂性。

## 附：课程要求完成情况总表

| 要求类别 | 课程要求 | 完成情况 |
| --- | --- | --- |
| 处理机调度 | 实现 1+n 种调度算法 | 已实现 FCFS、SJF、RR、Priority，并扩展 HRRN |
| 处理机调度 | 支持动态输入进程参数 | 已完成 |
| 处理机调度 | 输出运行顺序、周转时间等结果 | 已完成 |
| 处理机调度 | 分析算法特点与性能差异 | 已完成 |
| 内存管理 | 实现动态分区管理 | 已实现 FF、BF |
| 内存管理 | 实现页面置换机制 | 已实现 FIFO、LRU |
| 内存管理 | 展示分配与回收过程 | 已完成 |
| 内存管理 | 统计缺页次数与缺页率 | 已完成 |
| 进程同步 | 使用多线程模拟并发执行 | 已完成 |
| 进程同步 | 使用互斥锁、同步机制 | 已完成 |
| 进程同步 | 避免死锁与数据竞争 | 已完成 |
| 文件系统 | 支持创建、读写、删除 | 已完成 |
| 文件系统 | 实现空闲空间管理 | 已完成位示图管理 |
| 扩展提升 | 自主选题与创新实现 | 已完成调度性能测试与 HRRN 优化分析 |
| 工程实践 | Linux/g++/Makefile 编译运行 | 已完成 |
| 报告提交 | 包含代码 URL 与访问方式 | 需提交前填写 GitHub 地址 |

## 代码 URL 与访问方式

提交报告前请将以下内容替换为实际仓库地址：

```text
代码地址：https://github.com/<username>/os-course-design
访问方式：公开仓库，可直接访问；如为私有仓库，需提前向教师账号开放访问权限。
```
