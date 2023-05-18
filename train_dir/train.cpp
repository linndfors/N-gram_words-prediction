#include "ngram_predictor/ngram_predictor.hpp"
#include "memory_usage.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) 
{
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " path n " << std::endl;
        return 1;
    } 

    std::cout << "Training..." << std::endl;

    const auto path = argv[1];
    const auto n = std::stoi(argv[2]);

    auto ng = ngram_predictor{n};
    ng.read_corpus(path);
    ng.print_training_time();

    report_memory_usage();

    return 0;
}