/**
 * @brief 
 * @date 2021-05-30
 * 
 */
#pragma once
#ifndef DIRTBOX_TASK_MANAGER_H
#define DIRTBOX_TASK_MANAGER_H

#include <future>
#include <list>
#include <memory>
#include <thread>

#include <util/delegate.h>
#include <util/sharedqueue.h>

namespace dirtbox {

class Task {
public:
    virtual ~Task() {}
    virtual void run() = 0;
};

class DelegateTask : public Task {
public:
    using TaskFn = util::delegate<int(void)>;

    explicit DelegateTask(TaskFn fn) : f{std::move(fn)} {}

    void run() override {
        status = f();
    }

private:
    TaskFn f;
    int status = 0;
};

/**
 * @brief runs tasks on separate threads
 * 
 */
class TaskManager {
    util::SharedQueue<std::shared_ptr<Task>> tasks;
public:
    using TaskFnType = DelegateTask::TaskFn;

    TaskManager();
    ~TaskManager();

    void add_task(const std::shared_ptr<Task> fn);
    void add_task(const TaskFnType& fn);

private:
    void worker_fn(std::future<void> kill_notifier);

    std::promise<void> task_worker_kill_notifier;
    std::thread task_worker;
};
    
}

#endif // DIRTBOX_TASK_MANAGER_H