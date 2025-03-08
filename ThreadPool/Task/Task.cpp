#include "Task.h"

int Task::idCounter = 0;

Task::Task() : id(idCounter++), exactTask(generateTasks()) {
    std::lock_guard<std::mutex> lg(coutMutex);
    std::cout << "Task #" << id << " was created\n";
}

std::function<void()> Task::generateTasks() {
    std::random_device seed;
    std::mt19937 mt(seed());
    std::uniform_int_distribution distribution(4, 15);
    int taskTime = distribution(mt);

    std::function<void()> task = [taskTime]() {
        std::this_thread::sleep_for(std::chrono::seconds(taskTime));
    };

    return task;
}

int Task::getID() const {
    return this->id;
}

std::function<void()> const &Task::getExactTask() const {
    return this->exactTask;
}