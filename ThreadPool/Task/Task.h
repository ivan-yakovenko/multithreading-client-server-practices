#pragma once
#include <iostream>
#include <random>
#include <thread>
#include <mutex>

extern std::mutex coutMutex;

class Task {
private:
    int id;
    static int idCounter;
    std::function<void()> exactTask;
public:
    Task();
    static std::function<void()> generateTasks();
    [[nodiscard]] int getID() const;
    [[nodiscard]] std::function<void()> const &getExactTask() const;
};


