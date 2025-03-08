#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>
#include "../Task/Task.h"

using std::chrono::duration_cast;
using std::chrono::nanoseconds;

class ThreadPool {
private:
    std::queue<Task> taskQueue1, taskQueue2;
    std::vector<std::thread> workerThreads1, workerThreads2;
    std::mutex m1, m2;
    std::condition_variable cv1, cv2;
    bool stop;

    std::vector<long long> waitingTime1, waitingTime2;
    std::vector<int> waitingCount1, waitingCount2;
    long long execTime1, execTime2;
    int tasksAmount1, tasksAmount2;

public:
    ThreadPool();
    ~ThreadPool();
    void addTask(Task &task);
    void testCalculations();
};
