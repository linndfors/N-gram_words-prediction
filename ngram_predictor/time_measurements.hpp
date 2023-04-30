#ifndef COUNTWORDS_SEQ_TIME_MEASUREMENTS_HPP
#define COUNTWORDS_SEQ_TIME_MEASUREMENTS_HPP

#include <chrono>
#include <cassert>
#include <atomic>

inline std::chrono::steady_clock::time_point get_current_time_fenced() {
    assert(std::chrono::steady_clock::is_steady &&
                   "Timer should be steady (monotonic).");
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::steady_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

template<class D>
inline long long to_ms(const D& d)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}


#endif //COUNTWORDS_SEQ_TIME_MEASUREMENTS_HPP
