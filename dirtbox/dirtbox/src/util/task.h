/**
 
 * @author Hunter Borlik
 * @brief thread task
 * @version 0.1
 * @date 2021-03-12
 * 
 * 
 * 
 */
#pragma once
#ifndef UTIL_TASK_H
#define UTIL_TASK_H

#include <future>
#include <atomic>
#include <chrono>

#include "delegate.h"

namespace util {

class AsyncProgressTask {
public:
    using TaskFnType = delegate<void(std::atomic_uint32_t&, std::future<void>)>;

    AsyncProgressTask(TaskFnType task) : task_fn{task} {}

    void start();
    void notifyStop();
    /**
     * @brief returns true when task thread was successfully joined
     * 
     * @return true 
     * @return false 
     */
    bool join();
    bool waitFor(std::chrono::milliseconds t = std::chrono::milliseconds{0}) const;
    bool isDone() const;

    float getProgress() const {
        return task_progress.load() / (float)std::numeric_limits<uint32_t>::max();
    }

private:
    std::future<void> task_notifier{};
    std::promise<void> task_kill_notifier{};
    std::atomic_uint32_t task_progress{};

    TaskFnType task_fn;
};

}

#endif // UTIL_TASK_H