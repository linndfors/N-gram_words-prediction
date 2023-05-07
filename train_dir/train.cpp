#include "train.h"
#include <iostream>
#include <string>
#include <vector>
#include "ngram_predictor.hpp"
#include <sys/resource.h>

using ngram_t = std::vector<std::string>;

int main(int argc, char* argv[]) {
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
    ngram_predictor ng = ngram_predictor(path, n);
    ng.read_corpus();
    return 0;
}