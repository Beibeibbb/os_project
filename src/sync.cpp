#include "os_project.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace {
constexpr int buffer_size = 5;
constexpr int item_count = 10;

void tiny_sleep()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
}

void run_producer_consumer()
{
    std::array<int, buffer_size> buffer{};
    std::mutex mutex;
    std::mutex output_mutex;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    int in_pos = 0;
    int out_pos = 0;
    int count = 0;

    std::cout << "\n[生产者-消费者]\n";

    auto producer = [&] {
        for (int i = 0; i < item_count; ++i) {
            const int item = 100 + i;
            std::unique_lock<std::mutex> lock(mutex);
            not_full.wait(lock, [&] { return count < buffer_size; });
            buffer[in_pos] = item;
            {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "生产者 生产 " << item << " -> buffer[" << in_pos << "]\n";
            }
            in_pos = (in_pos + 1) % buffer_size;
            ++count;
            lock.unlock();
            not_empty.notify_one();
            tiny_sleep();
        }
    };

    auto consumer = [&] {
        for (int i = 0; i < item_count; ++i) {
            std::unique_lock<std::mutex> lock(mutex);
            not_empty.wait(lock, [&] { return count > 0; });
            const int item = buffer[out_pos];
            {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "消费者 消费 " << item << " <- buffer[" << out_pos << "]\n";
            }
            out_pos = (out_pos + 1) % buffer_size;
            --count;
            lock.unlock();
            not_full.notify_one();
            tiny_sleep();
        }
    };

    std::thread p(producer);
    std::thread c(consumer);
    p.join();
    c.join();
}

void run_readers_writers()
{
    std::mutex rw_mutex;
    std::mutex read_count_mutex;
    std::mutex output_mutex;
    int read_count = 0;
    int shared_data = 0;

    std::cout << "\n[读者-写者]\n";

    auto reader = [&](int id) {
        for (int i = 0; i < 3; ++i) {
            {
                std::lock_guard<std::mutex> count_lock(read_count_mutex);
                ++read_count;
                if (read_count == 1) {
                    rw_mutex.lock();
                }
            }

            {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "读者" << id << " 读取 shared_data=" << shared_data << '\n';
            }
            tiny_sleep();

            {
                std::lock_guard<std::mutex> count_lock(read_count_mutex);
                --read_count;
                if (read_count == 0) {
                    rw_mutex.unlock();
                }
            }
            tiny_sleep();
        }
    };

    auto writer = [&](int id) {
        for (int i = 0; i < 3; ++i) {
            std::lock_guard<std::mutex> write_lock(rw_mutex);
            shared_data += 10;
            {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "写者" << id << " 写入 shared_data=" << shared_data << '\n';
            }
            tiny_sleep();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(reader, i);
    }
    for (int i = 1; i <= 2; ++i) {
        threads.emplace_back(writer, i);
    }
    for (auto &thread : threads) {
        thread.join();
    }
}

void run_dining_philosophers()
{
    constexpr int philosopher_count = 5;
    std::array<std::mutex, philosopher_count> forks;
    std::mutex output_mutex;
    std::vector<std::thread> philosophers;

    std::cout << "\n[哲学家进餐]\n";

    auto philosopher = [&](int id) {
        const int left = id;
        const int right = (id + 1) % philosopher_count;
        const int first = std::min(left, right);
        const int second = std::max(left, right);

        for (int i = 0; i < 2; ++i) {
            {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "哲学家" << id << " 思考\n";
            }
            tiny_sleep();
            std::lock(forks[first], forks[second]);
            std::lock_guard<std::mutex> lock_first(forks[first], std::adopt_lock);
            std::lock_guard<std::mutex> lock_second(forks[second], std::adopt_lock);
            {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "哲学家" << id << " 进餐\n";
            }
            tiny_sleep();
        }
    };

    for (int i = 0; i < philosopher_count; ++i) {
        philosophers.emplace_back(philosopher, i);
    }
    for (auto &thread : philosophers) {
        thread.join();
    }
}
}

void run_sync_module()
{
    run_producer_consumer();
    run_readers_writers();
    run_dining_philosophers();
    std::cout << "\n同步分析说明:\n"
              << "生产者-消费者使用条件变量控制缓冲区空/满状态，互斥锁保护临界区。\n"
              << "读者-写者允许多个读者并发读，写者通过写锁获得独占访问。\n"
              << "哲学家进餐使用 std::lock 同时申请两把锁，避免死锁。\n";
    wait_for_enter();
}
