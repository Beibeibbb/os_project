# 操作系统课程设计

本项目按《操作系统》课程设计要求实现“基础必做”四类实验模块，并预留扩展方向。项目采用 C++17 编写，面向 Linux / Windows / WSL 的 g++ 环境，核心模块包括：

- 处理机调度：FCFS、SJF、时间片轮转 RR、优先级调度。
- 内存管理：动态分区首次适应 FF、最佳适应 BF、FIFO 页面置换、LRU 页面置换。
- 进程同步：生产者-消费者、读者-写者、哲学家进餐。
- 文件系统：文件创建、写入、读取、删除、目录表和空闲空间位示图。
- 扩展提升：调度性能测试与优化分析，包含 HRRN 改进算法与 CSV 结果导出。

## 编译运行

```bash
make
./os_course_design
```

也可以直接运行：

```bash
make run
```

清理编译产物：

```bash
make clean
```

如果当前环境没有 `make`，可以直接使用：

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude src/*.cpp -o os_course_design
```

## 项目结构

```text
.
├── include/os_project.h
├── src/main.cpp
├── src/scheduler.cpp
├── src/memory.cpp
├── src/sync.cpp
├── src/filesystem.cpp
├── src/extension.cpp
├── docs/report_template.md
└── Makefile
```

## 实验说明

程序启动后通过菜单选择不同模块。调度模块和内存页面置换模块支持默认样例和手动输入，便于对比不同算法性能；同步模块使用 `std::thread`、`std::mutex`、`std::condition_variable` 演示并发控制；文件系统模块提供默认演示和交互式命令。

扩展模块选择“调度与性能优化”方向。程序会自动构造 balanced 和 bursty 两组进程负载，对 FCFS、SJF、RR、Priority、HRRN 进行性能测试，输出平均等待时间、平均周转时间、平均响应时间、上下文切换次数、完成时长和吞吐率，并生成：

```text
docs/scheduler_benchmark.csv
```

## GitHub 提交说明

报告中需要写明代码托管 URL 和访问方式，例如：·

```text
代码地址：https://github.com/Beibeibbb/os_project
访问方式：公开仓库，可直接访问；或私有仓库，已向教师账号开放访问权限。
```
