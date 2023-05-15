/**
 * @brief 
 * @date 2021-06-02
 * 
 */
#pragma once
#ifndef DIRTBOX_EVENT_H
#define DIRTBOX_EVENT_H

#include <condition_variable>

namespace dirtbox {

template<typename T>
class ThreadEvent {
public:
    void fire(const T& value) {
        std::unique_lock<std::mutex> lck(mtx);
        ready += 1;
        cv.notify_all();
        this->value = value;
    }

    const T wait() {
        std::unique_lock<std::mutex> lck(mtx);
        int val = ready;

        cv.wait(lck, [this, val]() {return ready != val;});

        return value;
    }

private:
    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    int ready;
    T value;
};

}

#endif // DIRTBOX_EVENT_H