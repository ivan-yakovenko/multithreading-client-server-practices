#include "ThreadPool.h"

std::mutex coutMutex;

void ThreadPool::addTask(Task &task) {
    std::random_device seed;
    std::mt19937 mt(seed());
    std::uniform_int_distribution distribution(1, 2);
    int n = distribution(mt);

    if (n == 1) {
        {
            std::lock_guard<std::mutex> lg1(m1);
            taskQueue1.push(task);
        }
        cv1.notify_one();
        {
            std::lock_guard<std::mutex> lg(coutMutex);
            std::cout << "Task #" << task.getID() << " was added to the first queue\n";
        }
    } else {
        {
            std::lock_guard<std::mutex> lg2(m2);
            taskQueue2.push(task);
        }
        cv2.notify_one();
        {
            std::lock_guard<std::mutex> lg(coutMutex);
            std::cout << "Task #" << task.getID() << " was added to the second queue\n";
        }
    }
}

ThreadPool::ThreadPool() : stop(false), execTime1(0), execTime2(0), tasksAmount1(0), tasksAmount2(0) {
    workerThreads1.reserve(2);
    workerThreads2.reserve(2);
    waitingTime1.resize(2, 0);
    waitingTime2.resize(2, 0);
    waitingCount1.resize(2, 0);
    waitingCount2.resize(2, 0);

    for (int i = 0; i < workerThreads1.capacity(); i++) {
        workerThreads1.emplace_back([this, i]() {
            while (true) {
                std::unique_lock<std::mutex> ul1(m1);
                auto waitingStartTimer = std::chrono::high_resolution_clock::now();
                cv1.wait(ul1, [this]() {
                    return stop || !taskQueue1.empty();
                });
                auto waitingEndTimer = std::chrono::high_resolution_clock::now();
                auto waitTime =  duration_cast<nanoseconds>(waitingEndTimer - waitingStartTimer);
                waitingTime1[i] += waitTime.count();
                waitingCount1[i]++;
                if (stop && taskQueue1.empty()) {
                    return;
                }
                std::function<void()> task = (taskQueue1.front().getExactTask());
                int id = taskQueue1.front().getID();
                taskQueue1.pop();
                ul1.unlock();
                {
                    std::lock_guard<std::mutex> lg(coutMutex);
                    std::cout << "Worker thread #" << i << " is executing a task #" << id << " from the first queue\n";
                }
                auto startTimer = std::chrono::high_resolution_clock::now();
                task();
                auto endTimer = std::chrono::high_resolution_clock::now();
                auto execTime =  duration_cast<nanoseconds>(endTimer - startTimer);
                execTime1 += execTime.count();
                tasksAmount1++;
                {
                    std::lock_guard<std::mutex> lg(coutMutex);
                    std::cout << "Worker thread #" << i << " executed a task #" << id << " successfully from the first queue in " << double(execTime.count()) * 1e-9 << " seconds\n";
                }
            }
        });
    }
    for (int i = 0; i < workerThreads2.capacity(); i++) {
        workerThreads2.emplace_back([this, i]() {
            while (true) {
                std::unique_lock<std::mutex> ul2(m2);
                auto waitingStartTimer = std::chrono::high_resolution_clock::now();
                cv2.wait(ul2, [this]() {
                    return stop || !taskQueue2.empty();
                });
                auto waitingEndTimer = std::chrono::high_resolution_clock::now();
                auto waitTime =  duration_cast<nanoseconds>(waitingEndTimer - waitingStartTimer);
                waitingTime2[i] += waitTime.count();
                waitingCount2[i]++;
                if (stop && taskQueue2.empty()) {
                    return;
                }
                std::function<void()> task = (taskQueue2.front().getExactTask());
                int id = taskQueue2.front().getID();
                taskQueue2.pop();
                ul2.unlock();
                {
                    std::lock_guard<std::mutex> lg(coutMutex);
                    std::cout << "Worker thread #" << i << " is executing a task #" << id << " from the second queue\n";
                }
                auto startTimer = std::chrono::high_resolution_clock::now();
                task();
                auto endTimer = std::chrono::high_resolution_clock::now();
                auto execTime =  duration_cast<nanoseconds>(endTimer - startTimer);
                execTime2 += execTime.count();
                tasksAmount2++;
                {
                    std::lock_guard<std::mutex> lg(coutMutex);
                    std::cout << "Worker thread #" << i << " executed a task #" << id << " successfully from the second queue in " << double(execTime.count()) * 1e-9 << " seconds\n";
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {

    {
        std::lock_guard<std::mutex> lg1(m1);
        stop = true;
    }

    cv1.notify_all();

    for (auto &thread: workerThreads1) {
        thread.join();
    }

    {
        std::lock_guard<std::mutex> lg2(m2);
        stop = true;
    }

    cv2.notify_all();

    for (auto &thread: workerThreads2) {
        thread.join();
    }

    testCalculations();
}

void ThreadPool::testCalculations() {
    for(int i = 0; i < workerThreads1.size(); i++) {
        double averageWaitTime = (double(waitingTime1[i]) / waitingCount1[i]) * 1e-9;
        std::cout << "Average time a thread #" << i << " from the first queue is in the waiting state " << averageWaitTime << " seconds\n";
    }

    for(int i = 0; i < workerThreads2.size(); i++) {
        double averageWaitTime = (double(waitingTime2[i]) / waitingCount2[i]) * 1e-9;
        std::cout << "Average time a thread #" << i << " from the second queue is in the waiting state " << averageWaitTime << " seconds\n";
    }

    double averageQueueLength1 = double(tasksAmount1) / double(workerThreads1.size());
    double averageQueueLength2 = double(tasksAmount2) / double(workerThreads2.size());

    double averageExecTime1 = (double(execTime1) / tasksAmount1) * 1e-9;
    double averageExecTime2 = (double(execTime2) / tasksAmount2) * 1e-9;

    std::cout << "Average length of the first queue: " << averageQueueLength1 << "\n";
    std::cout << "Average length of the second queue: " << averageQueueLength2 << "\n";

    std::cout << "Average task execution time for the first queue: " << averageExecTime1 << " seconds\n";
    std::cout << "Average task execution time for the second queue: " << averageExecTime2 << " seconds\n";



}