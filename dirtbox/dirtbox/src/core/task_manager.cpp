#include <core/task_manager.h>

#include <thread>

namespace dirtbox {

TaskManager::TaskManager() :
    task_worker_kill_notifier{},
    task_worker{&TaskManager::worker_fn, this, task_worker_kill_notifier.get_future()} {

}

TaskManager::~TaskManager() {
    task_worker_kill_notifier.set_value();
    task_worker.join();
}

void TaskManager::add_task(const std::shared_ptr<Task> fn) {
    tasks.push_back(fn);
}

void TaskManager::add_task(const TaskFnType& fn) {
    tasks.push_back(std::make_shared<DelegateTask>(fn));
}

void TaskManager::worker_fn(std::future<void> kill_notifier) {
    while(kill_notifier.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
        auto task = tasks.pop_front_wait(std::chrono::milliseconds(1));
        if (task)
            (*task)->run();
        else
            std::this_thread::yield();
        
    }
}

}