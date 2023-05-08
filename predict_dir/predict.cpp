//
// Created by user on 07.05.23.
//

#include <iostream>
#include <string>
#include <vector>
#include "ngram_predictor.hpp"
#include <sys/resource.h>

using ngram_t = std::vector<std::string>;

int main(int argc, char* argv[]) {
    std::cout << "Predicting..." << std::endl;
    std::string path;
    int n, num_words_to_predict;
    ngram_t context;
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " n amount_of_words context*" << std::endl;
        exit(1);
    } else {
        n = std::stoi(argv[1]);
        num_words_to_predict = std::stoi(argv[2]);
        if (argc > 3) {
            // add <s> to start of context so its length is at least n-1
            for (int i = 0; i < (n - 1) - (argc - 3); ++i) {
                context.emplace_back("<s>");
            }

            for(int i = 3; i < argc; ++i) {
                context.emplace_back(argv[i]);
            }
        } else {
            context.resize(n-1, "<s>");
        }
    }

    ngram_predictor ng = ngram_predictor(n);
    ngram_predictor::print_list(ng.predict_words(num_words_to_predict, context));
    std::cout << std::endl;

    ng.print_predicting_time();
    std::cout << std::endl;

    struct rusage r_usage{};
    getrusage(RUSAGE_SELF, &r_usage);
    auto max_mem = r_usage.ru_maxrss;
    std::cout << "Max memory usage: " << max_mem << " KB ("
              << max_mem/1000 << " MB/"<< max_mem/1000000 << " GB)" << std::endl;
    return 0;
}