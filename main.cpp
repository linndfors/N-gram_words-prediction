#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "ngram_predictor.hpp"

using ngram_t = std::vector<std::string>;

int main(int argc, char* argv[]) {
    std::string path;
    int n, num_words_to_predict;
    ngram_t context;
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " path n num_words_to_predict context*" << std::endl;
        return 1;
    } else {
        path = argv[1];
        n = std::stoi(argv[2]);
        num_words_to_predict = std::stoi(argv[3]);

        if (argc > 4) {
            context.emplace_back("<s>");
            for(int i = 4; i < argc; ++i) {
                context.emplace_back(argv[i]);
            }
        } else {
            context.resize(n-1, "<s>");
        }
    }


    ngram ng = ngram(path, n);
    ngram::print_list(ng.predict_words(num_words_to_predict, context));
    return 0;
}
