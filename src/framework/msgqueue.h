#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

/// \brief
/// Thread safe queue.
///
/// By design, there is no method named `pop`.
/// To retrieve an object from the queue, you must
/// use either the non-blocking MsgQueue::fetch
/// or the blocking MsgQueue::pop_blocking method.
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

    /// Add object at the end of the queue.
    void push(T t) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(t);
        cond.notify_one();
    }

    /// Return true if the queue is empty.
    bool empty() {
        std::unique_lock<std::mutex> lock(mutex);
        return queue.empty();
    }

    /// \brief
    /// Wait until there is an object in the queue, then
    /// remove and return the first object.
    T pop_blocking() {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.empty()) {
            cond.wait(lock);
        }
        T val = queue.front();
        queue.pop();
        return val;
    }

    /// \brief
    /// A non-blocking pop.
    ///
    /// If the queue is empty, return false.
    /// Otherwise remove the first object from the queue,
    /// assign it to `val`, and return true.
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
