#include "task.h"

#include <iostream>

namespace util {

void AsyncProgressTask::start() {
    if(isDone()) {
        task_progress.store(0);
        task_kill_notifier = {};
        task_notifier = std::async(std::launch::async, [&](){
            task_fn(task_progress, task_kill_notifier.get_future());
        });
    }
}

void AsyncProgressTask::notifyStop() {
    task_kill_notifier.set_value();
}

bool AsyncProgressTask::join() {
    if (task_notifier.valid() && isDone()) {
        task_notifier.get();
        task_notifier = {};
        return true;
    }
    return false;
}

bool AsyncProgressTask::waitFor(std::chrono::milliseconds t) const {
    if (task_notifier.valid())
        return task_notifier.wait_for(t) == std::future_status::ready;
    return false;
}

bool AsyncProgressTask::isDone() const {
    if (task_notifier.valid())
        return task_notifier.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    return true;
}

}