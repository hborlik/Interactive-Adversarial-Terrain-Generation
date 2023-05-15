/**
 * @author Hunter Borlik
 * @brief Thread safe queue
 * @version 0.2
 * @date 2020-08-14
 * 
 * updated 2020-01-21: header guard name change, file name change
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#pragma once
#ifndef UTIL_SHARED_QUEUE_H_HEADER_GUARD
#define UTIL_SHARED_QUEUE_H_HEADER_GUARD

#include <deque>
#include <mutex>
#include <optional>
#include <chrono>
#include <condition_variable>

namespace util {

/**
 * @brief Thread safe queue
 * 
 * @tparam T 
 */
template<typename T>
class SharedQueue {
public:
    SharedQueue();
    ~SharedQueue();

    bool empty() const;
    size_t size() const;

    T front() const;
    T pop_front();

    template<class Rep, class Period>
    std::optional<T> pop_front_wait(const std::chrono::duration<Rep, Period>& dur);

    void push_back(const T& item);
    void push_back(T&& item);

private:
    std::deque<T> queue;
    mutable std::mutex mutex;
    mutable std::condition_variable cond;
};

template<typename T>
SharedQueue<T>::SharedQueue() {}

template<typename T>
SharedQueue<T>::~SharedQueue() {}

template<typename T>
bool SharedQueue<T>::empty() const {
    std::unique_lock<std::mutex> lock{mutex};
    return queue.empty();
}

template<typename T>
size_t SharedQueue<T>::size() const {
    std::unique_lock<std::mutex> lock{mutex};
    return queue.size();
}

template<typename T>
T SharedQueue<T>::front() const {
    std::unique_lock<std::mutex> lock{mutex};
    while(queue.empty())
        cond.wait(lock);
    return queue.front();
}

template<typename T>
T SharedQueue<T>::pop_front() {
    std::unique_lock<std::mutex> lock{mutex};
    while(queue.empty())
        cond.wait(lock);
    auto r{std::move(queue.front())};
    queue.pop_front();
    return r;
}

template<typename T>
template<class Rep, class Period>
std::optional<T> SharedQueue<T>::pop_front_wait(const std::chrono::duration<Rep, Period>& dur) {
    std::unique_lock<std::mutex> lock{mutex};
    while(queue.empty())
        if (cond.wait_for(lock, dur) == std::cv_status::timeout)
            return {};
    auto r{std::move(queue.front())};
    queue.pop_front();
    return r;
}

template<typename T>
void SharedQueue<T>::push_back(const T& item) {
    {
        std::unique_lock<std::mutex> lock{mutex};
        queue.push_back(item);
    }
    cond.notify_one();
}

template<typename T>
void SharedQueue<T>::push_back(T&& item) {
    {
        std::unique_lock<std::mutex> lock{mutex};
        queue.push_back(std::move(item));
    }
    cond.notify_one();
}

}

#endif // UTIL_SHARED_QUEUE_H_HEADER_GUARD