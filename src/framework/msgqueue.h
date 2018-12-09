#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class MsgQueue {
public:
    MsgQueue() :
        queue(),
        mutex(),
        cond() {
    }

    ~MsgQueue() {
    }

    void push(T t) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(t);
        cond.notify_one();
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(mutex);
        return queue.empty();
    }

    T pop_blocking() {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.empty()) {
            cond.wait(lock);
        }
        T val = queue.front();
        queue.pop();
        return val;
    }

    bool fetch(T &val)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.empty())
            return false;
        val = queue.front();
        queue.pop();
        return true;
    }

private:
    std::queue<T> queue;
    mutable std::mutex mutex;
    std::condition_variable cond;
};
