#include <iostream>
#include <string>
#include <vector>
#include "ngram_predictor.hpp"
#include <sys/resource.h>

using ngram_t = std::vector<std::string>;

int main(int argc, char* argv[]) {
    std::cout << "Training..." << std::endl;
    std::string path;
    int n;
    ngram_t context;
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " path n " << std::endl;
        exit(1);
    } else {
        path = argv[1];
        n = std::stoi(argv[2]);
    }
    ngram_predictor ng = ngram_predictor(n);
    ng.read_corpus(path);

    ng.print_training_time();
    std::cout << std::endl;

    struct rusage r_usage{};
    getrusage(RUSAGE_SELF, &r_usage);
    auto max_mem = r_usage.ru_maxrss;
    std::cout << "Max memory usage: " << max_mem << " KB ("
              << max_mem/1000 << " MB/"<< max_mem/1000000 << " GB)" << std::endl;
    return 0;
}