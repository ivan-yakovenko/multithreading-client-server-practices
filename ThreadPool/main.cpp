#include <iostream>
#include "ThreadPool/ThreadPool.h"
#include "Task/Task.h"

int main() {
    ThreadPool threadPool;
    auto startingTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::microseconds(400);

    std::thread taskGeneratorThread([&threadPool, startingTime, duration]() {
        while(std::chrono::high_resolution_clock::now() - startingTime < duration) {
            Task task;
            threadPool.addTask(task);
        }
        {
            std::lock_guard<std::mutex> lg(coutMutex);
            std::cout << "Time limit reached, no more tasks are generated\n";
        }
    });

    taskGeneratorThread.join();

}