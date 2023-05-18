#include "memory_usage.hpp"

#include <sys/resource.h>
#include <iostream>

auto report_memory_usage() -> void
{
    struct rusage r_usage{};
    getrusage(RUSAGE_SELF, &r_usage);

    const auto max_mem = r_usage.ru_maxrss;
    std::cout << "Max memory usage: " << max_mem << " KB (" << max_mem/1000 << " MB/"<< max_mem/1000000 << " GB)" << std::endl;
}