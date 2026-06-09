#include "os_project.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <queue>
#include <vector>

namespace {
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

void reset(std::vector<Process> &processes)
{
    for (auto &p : processes) {
        p.remaining = p.burst;
        p.start = -1;
        p.finish = 0;
        p.completed = false;
    }
}

void print_result(const std::string &name, std::vector<Process> processes)
{
    double total_wait = 0.0;
    double total_turnaround = 0.0;

    std::cout << "\n[" << name << "] 统计结果\n";
    std::cout << "PID\t到达\t服务\t优先级\t完成\t等待\t周转\n";
    for (const auto &p : processes) {
        const int turnaround = p.finish - p.arrival;
        const int waiting = turnaround - p.burst;
        total_wait += waiting;
        total_turnaround += turnaround;
        std::cout << "P" << p.pid << '\t' << p.arrival << '\t' << p.burst << '\t'
                  << p.priority << '\t' << p.finish << '\t' << waiting << '\t'
                  << turnaround << '\n';
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "平均等待时间: " << total_wait / processes.size() << '\n';
    std::cout << "平均周转时间: " << total_turnaround / processes.size() << '\n';
}

bool all_done(const std::vector<Process> &processes)
{
    return std::all_of(processes.begin(), processes.end(), [](const Process &p) {
        return p.completed;
    });
}

void run_fcfs(std::vector<Process> processes)
{
    reset(processes);
    int time = 0;
    int finished = 0;
    std::cout << "\n[FCFS] 运行顺序: ";

    while (finished < static_cast<int>(processes.size())) {
        int chosen = -1;
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            const auto &p = processes[i];
            if (!p.completed && p.arrival <= time &&
                (chosen == -1 || p.arrival < processes[chosen].arrival ||
                 (p.arrival == processes[chosen].arrival && p.pid < processes[chosen].pid))) {
                chosen = i;
            }
        }

        if (chosen == -1) {
            ++time;
            continue;
        }

        std::cout << "P" << processes[chosen].pid << ' ';
        processes[chosen].start = time;
        time += processes[chosen].burst;
        processes[chosen].remaining = 0;
        processes[chosen].finish = time;
        processes[chosen].completed = true;
        ++finished;
    }
    std::cout << '\n';
    print_result("FCFS", processes);
}

void run_sjf(std::vector<Process> processes)
{
    reset(processes);
    int time = 0;
    int finished = 0;
    std::cout << "\n[SJF 非抢占] 运行顺序: ";

    while (finished < static_cast<int>(processes.size())) {
        int chosen = -1;
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            const auto &p = processes[i];
            if (!p.completed && p.arrival <= time &&
                (chosen == -1 || p.burst < processes[chosen].burst ||
                 (p.burst == processes[chosen].burst && p.arrival < processes[chosen].arrival))) {
                chosen = i;
            }
        }

        if (chosen == -1) {
            ++time;
            continue;
        }

        std::cout << "P" << processes[chosen].pid << ' ';
        processes[chosen].start = time;
        time += processes[chosen].burst;
        processes[chosen].finish = time;
        processes[chosen].completed = true;
        ++finished;
    }
    std::cout << '\n';
    print_result("SJF", processes);
}

void run_priority(std::vector<Process> processes)
{
    reset(processes);
    int time = 0;
    int finished = 0;
    std::cout << "\n[优先级 非抢占，数值越小优先级越高] 运行顺序: ";

    while (finished < static_cast<int>(processes.size())) {
        int chosen = -1;
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            const auto &p = processes[i];
            if (!p.completed && p.arrival <= time &&
                (chosen == -1 || p.priority < processes[chosen].priority ||
                 (p.priority == processes[chosen].priority && p.arrival < processes[chosen].arrival))) {
                chosen = i;
            }
        }

        if (chosen == -1) {
            ++time;
            continue;
        }

        std::cout << "P" << processes[chosen].pid << ' ';
        processes[chosen].start = time;
        time += processes[chosen].burst;
        processes[chosen].finish = time;
        processes[chosen].completed = true;
        ++finished;
    }
    std::cout << '\n';
    print_result("Priority", processes);
}

void run_rr(std::vector<Process> processes, int quantum)
{
    reset(processes);
    std::queue<int> ready;
    std::vector<bool> in_queue(processes.size(), false);
    int time = 0;

    std::cout << "\n[RR 时间片=" << quantum << "] 运行顺序: ";
    while (!all_done(processes)) {
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            if (!processes[i].completed && !in_queue[i] && processes[i].arrival <= time) {
                ready.push(i);
                in_queue[i] = true;
            }
        }

        if (ready.empty()) {
            ++time;
            continue;
        }

        const int idx = ready.front();
        ready.pop();
        in_queue[idx] = false;
        if (processes[idx].start == -1) {
            processes[idx].start = time;
        }

        const int slice = std::min(processes[idx].remaining, quantum);
        std::cout << "P" << processes[idx].pid << "(" << time << "-" << time + slice << ") ";
        time += slice;
        processes[idx].remaining -= slice;

        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            if (i != idx && !processes[i].completed && !in_queue[i] && processes[i].arrival <= time) {
                ready.push(i);
                in_queue[i] = true;
            }
        }

        if (processes[idx].remaining == 0) {
            processes[idx].finish = time;
            processes[idx].completed = true;
        } else {
            ready.push(idx);
            in_queue[idx] = true;
        }
    }
    std::cout << '\n';
    print_result("RR", processes);
}

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
    processes.reserve(n);
    for (int i = 0; i < n; ++i) {
        Process p;
        p.pid = i + 1;
        std::cout << "\nP" << p.pid << ":\n";
        p.arrival = read_int("到达时间: ", 0, 10000);
        p.burst = read_int("服务时间: ", 1, 10000);
        p.priority = read_int("优先级(数字越小越高): ", 1, 10000);
        processes.push_back(p);
    }
    return processes;
}
}

void run_scheduler_module()
{
    const auto processes = load_processes();
    const int quantum = read_int("RR 时间片大小: ", 1, 1000);

    run_fcfs(processes);
    run_sjf(processes);
    run_rr(processes, quantum);
    run_priority(processes);

    std::cout << "\n算法对比说明:\n"
              << "FCFS 实现简单但容易产生短作业等待长作业的护航效应。\n"
              << "SJF 平均等待时间通常较低，但需要预知服务时间，长作业可能饥饿。\n"
              << "RR 响应性较好，适合分时系统，但时间片过小会增加切换开销。\n"
              << "优先级调度能体现任务重要性，但低优先级任务可能长期等待。\n";
    wait_for_enter();
}
