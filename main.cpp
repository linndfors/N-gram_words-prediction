#include "ngram_predictor/ngram_predictor.hpp"
#include "memory_usage.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>

auto operator<<(std::ostream& os, const ngram_predictor::ngrams& words) -> std::ostream&
{
    std::copy(words.begin(), words.end(), std::ostream_iterator<ngram_predictor::word>{os, " "});
    return os;
}

int main(int argc, char* argv[]) 
{
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " path n num_words_to_predict context*" << std::endl;
        return 1;
    } 
    
    const auto path = argv[1];
    const n = std::stoi(argv[2]);
    const auto num_words_to_predict = std::stoi(argv[3]);

    auto context = ngram_predictor::ngrams{};
    if (argc > 4) {
        // add <s> to start of context so its length is at least n-1
        for (int i = 0; i < (n - 1) - (argc - 4); ++i) {
            context.emplace_back("<s>");
        }

        for(int i = 4; i < argc; ++i) {
            context.emplace_back(argv[i]);
        }
    } else {
        context.resize(n-1, "<s>");
    }

    auto ng = ngram_predictor{path, n};
    std::cout << ng.predict_words(num_words_to_predict, context) << std::endl;

    ng.print_training_time();
    ng.print_predicting_time();
    ng.print_writing_words_time();

    report_memory_usage();

    return 0;
}
