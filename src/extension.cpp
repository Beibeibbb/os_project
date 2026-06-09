#include "os_project.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
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

std::vector<Process> balanced_workload()
{
    return {
        {1, 0, 6, 3}, {2, 1, 2, 1}, {3, 2, 8, 4}, {4, 3, 3, 2},
        {5, 4, 4, 3}, {6, 6, 5, 2}, {7, 8, 2, 5}, {8, 9, 7, 1},
    };
}

std::vector<Process> bursty_workload()
{
    return {
        {1, 0, 12, 4}, {2, 1, 1, 1}, {3, 2, 2, 2}, {4, 3, 1, 3},
        {5, 5, 9, 5},  {6, 6, 2, 1}, {7, 7, 1, 2}, {8, 9, 8, 4},
    };
}

void reset(std::vector<Process> &processes)
{
    for (auto &p : processes) {
        p.remaining = p.burst;
        p.start = -1;
        p.finish = 0;
        p.completed = false;
    }
}

bool all_done(const std::vector<Process> &processes)
{
    return std::all_of(processes.begin(), processes.end(), [](const Process &p) {
        return p.completed;
    });
}

Metrics calculate_metrics(const std::string &workload,
                          const std::string &algorithm,
                          const std::vector<Process> &processes,
                          int context_switches)
{
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;
    int first_arrival = processes.front().arrival;
    int last_finish = 0;

    for (const auto &p : processes) {
        const int turnaround = p.finish - p.arrival;
        const int waiting = turnaround - p.burst;
        const int response = p.start - p.arrival;
        total_waiting += waiting;
        total_turnaround += turnaround;
        total_response += response;
        first_arrival = std::min(first_arrival, p.arrival);
        last_finish = std::max(last_finish, p.finish);
    }

    const int makespan = last_finish - first_arrival;
    return {
        workload,
        algorithm,
        total_waiting / processes.size(),
        total_turnaround / processes.size(),
        total_response / processes.size(),
        context_switches,
        makespan,
        processes.size() / static_cast<double>(makespan),
    };
}

Metrics simulate_non_preemptive(std::vector<Process> processes,
                                const std::string &workload,
                                const std::string &algorithm)
{
    reset(processes);
    int time = 0;
    int completed = 0;
    int context_switches = 0;
    int last_pid = -1;

    while (completed < static_cast<int>(processes.size())) {
        int chosen = -1;
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            if (processes[i].completed || processes[i].arrival > time) {
                continue;
            }

            bool better = false;
            if (chosen == -1) {
                better = true;
            } else if (algorithm == "FCFS") {
                better = processes[i].arrival < processes[chosen].arrival;
            } else if (algorithm == "SJF") {
                better = processes[i].burst < processes[chosen].burst;
            } else if (algorithm == "Priority") {
                better = processes[i].priority < processes[chosen].priority;
            } else if (algorithm == "HRRN") {
                const double wait_i = time - processes[i].arrival;
                const double wait_chosen = time - processes[chosen].arrival;
                const double ratio_i = (wait_i + processes[i].burst) / processes[i].burst;
                const double ratio_chosen = (wait_chosen + processes[chosen].burst) / processes[chosen].burst;
                better = ratio_i > ratio_chosen;
            }

            if (better || (chosen != -1 && processes[i].pid < processes[chosen].pid &&
                           processes[i].arrival == processes[chosen].arrival)) {
                chosen = i;
            }
        }

        if (chosen == -1) {
            ++time;
            continue;
        }

        if (last_pid != -1 && last_pid != processes[chosen].pid) {
            ++context_switches;
        }
        last_pid = processes[chosen].pid;
        processes[chosen].start = time;
        time += processes[chosen].burst;
        processes[chosen].finish = time;
        processes[chosen].completed = true;
        ++completed;
    }

    return calculate_metrics(workload, algorithm, processes, context_switches);
}

Metrics simulate_rr(std::vector<Process> processes, const std::string &workload, int quantum)
{
    reset(processes);
    std::vector<int> ready;
    std::vector<bool> in_queue(processes.size(), false);
    int time = 0;
    int last_pid = -1;
    int context_switches = 0;

    while (!all_done(processes)) {
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            if (!processes[i].completed && !in_queue[i] && processes[i].arrival <= time) {
                ready.push_back(i);
                in_queue[i] = true;
            }
        }

        if (ready.empty()) {
            ++time;
            continue;
        }

        const int idx = ready.front();
        ready.erase(ready.begin());
        in_queue[idx] = false;

        if (last_pid != -1 && last_pid != processes[idx].pid) {
            ++context_switches;
        }
        last_pid = processes[idx].pid;

        if (processes[idx].start == -1) {
            processes[idx].start = time;
        }

        const int slice = std::min(quantum, processes[idx].remaining);
        time += slice;
        processes[idx].remaining -= slice;

        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            if (i != idx && !processes[i].completed && !in_queue[i] && processes[i].arrival <= time) {
                ready.push_back(i);
                in_queue[i] = true;
            }
        }

        if (processes[idx].remaining == 0) {
            processes[idx].finish = time;
            processes[idx].completed = true;
        } else {
            ready.push_back(idx);
            in_queue[idx] = true;
        }
    }

    return calculate_metrics(workload, "RR(q=" + std::to_string(quantum) + ")", processes, context_switches);
}

void print_processes(const std::string &name, const std::vector<Process> &processes)
{
    std::cout << "\n[" << name << "] 测试进程序列\n";
    std::cout << "PID\t到达\t服务\t优先级\n";
    for (const auto &p : processes) {
        std::cout << "P" << p.pid << '\t' << p.arrival << '\t' << p.burst << '\t' << p.priority << '\n';
    }
}

void print_metrics(const std::vector<Metrics> &results)
{
    std::cout << "\n调度性能测试结果\n";
    std::cout << "负载\t算法\t平均等待\t平均周转\t平均响应\t切换次数\t完成时长\t吞吐率\n";

    std::cout << std::fixed << std::setprecision(2);
    for (const auto &m : results) {
        std::cout << m.workload << '\t' << m.algorithm << '\t'
                  << m.average_waiting << '\t' << m.average_turnaround << '\t'
                  << m.average_response << '\t' << m.context_switches << '\t'
                  << m.makespan << '\t' << m.throughput << '\n';
    }
}

void write_csv(const std::vector<Metrics> &results)
{
    std::ofstream out("docs/scheduler_benchmark.csv");
    out << "workload,algorithm,average_waiting,average_turnaround,average_response,"
           "context_switches,makespan,throughput\n";
    out << std::fixed << std::setprecision(2);
    for (const auto &m : results) {
        out << m.workload << ',' << m.algorithm << ',' << m.average_waiting << ','
            << m.average_turnaround << ',' << m.average_response << ',' << m.context_switches
            << ',' << m.makespan << ',' << m.throughput << '\n';
    }
}

void analyze_best(const std::vector<Metrics> &results)
{
    const auto best_wait = std::min_element(results.begin(), results.end(), [](const Metrics &a, const Metrics &b) {
        return a.average_waiting < b.average_waiting;
    });
    const auto best_response = std::min_element(results.begin(), results.end(), [](const Metrics &a, const Metrics &b) {
        return a.average_response < b.average_response;
    });

    std::cout << "\n扩展实验结论:\n";
    std::cout << "1. HRRN 是本扩展模块加入的改进算法，响应比=(等待时间+服务时间)/服务时间。\n";
    std::cout << "2. HRRN 会随着等待时间增长提高长作业优先级，可缓解 SJF 对长作业不友好的问题。\n";
    std::cout << "3. RR 的平均响应时间通常较低，但上下文切换次数明显增加，体现了响应性与切换开销的权衡。\n";
    std::cout << "4. 本次测试中平均等待时间最低的是 " << best_wait->workload << " / " << best_wait->algorithm
              << "，平均响应时间最低的是 " << best_response->workload << " / " << best_response->algorithm << "。\n";
}
}

void run_extension_module()
{
    const int quantum = read_int("请输入 RR 性能测试时间片(建议 2-4): ", 1, 20);
    const auto balanced = balanced_workload();
    const auto bursty = bursty_workload();
    std::vector<Metrics> results;

    print_processes("balanced", balanced);
    print_processes("bursty", bursty);

    for (const auto &item : std::vector<std::pair<std::string, std::vector<Process>>>{
             {"balanced", balanced},
             {"bursty", bursty},
         }) {
        results.push_back(simulate_non_preemptive(item.second, item.first, "FCFS"));
        results.push_back(simulate_non_preemptive(item.second, item.first, "SJF"));
        results.push_back(simulate_rr(item.second, item.first, quantum));
        results.push_back(simulate_non_preemptive(item.second, item.first, "Priority"));
        results.push_back(simulate_non_preemptive(item.second, item.first, "HRRN"));
    }

    print_metrics(results);
    write_csv(results);
    analyze_best(results);
    std::cout << "\n已生成报告数据文件: docs/scheduler_benchmark.csv\n";
    wait_for_enter();
}
