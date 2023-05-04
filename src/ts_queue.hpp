// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef INTEGRATE_PARALLEL_QUEUE_TS_QUEUE_H
#define INTEGRATE_PARALLEL_QUEUE_TS_QUEUE_H

#include <iostream>
#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T>
class ts_queue {
    std::deque<T> queue;
    std::mutex data_mutex;
    std::condition_variable not_empty_cond_var;
    std::condition_variable not_full_cond_var;

    size_t max_size;
public:
    explicit ts_queue(size_t max_size) : max_size(max_size) {};
     ~ts_queue()  = default;
    ts_queue(const ts_queue&)  = default;
    ts_queue& operator=(const ts_queue<T>&) = delete;


    void push(T&& value) {
        {
            std::unique_lock<std::mutex> lck(data_mutex);
            not_full_cond_var.wait(lck, [this] { return queue.size() < max_size; });
            queue.emplace_back(std::forward<T>(value));
        }
        not_empty_cond_var.notify_all();
    }

    T pop() {
        T item;
        {
            std::unique_lock<std::mutex> lck(data_mutex);
            not_empty_cond_var.wait(lck, [this] { return !queue.empty(); });
            item = std::move(queue.front());
            queue.pop_front();
        }
        not_full_cond_var.notify_all();
        return item;
    }

    [[nodiscard]] size_t size() const {
        return queue.size();
    }
};


#endif //INTEGRATE_PARALLEL_QUEUE_TS_QUEUE_H
