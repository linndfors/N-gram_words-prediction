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
        std::cout << "Usage: " << argv[0] << " train path n" << std::endl;
        std::cout<<"---------OR---------"<<std::endl;
        std::cout << "Usage: " << argv[0] << " predict n num_words_to_predict context*" << std::endl;
        return 1;
    }
    const auto type = argv[1];
    if (strcmp(type, "train") == 0) {
        std::cout << "Training..." << std::endl;

        const auto path = argv[2];
        const auto n = std::stoi(argv[3]);

        auto ng = ngram_predictor{n};
        ng.read_corpus(path);
        ng.print_training_time();
    }
    else if (strcmp(type,"predict") == 0) {
        std::cout << "Predicting..." << std::endl;

        const auto n = std::stoi(argv[2]);
        const auto num_words_to_predict = std::stoi(argv[3]);

        auto context = ngram_predictor::ngrams{};
        if (argc > 4) {
            // add <s> to start of context so its length is at least n-1
            for (int i = 0; i < (n - 1) - (argc - 4); ++i) {
                context.emplace_back("<s>");
            }

            for (int i = 4; i < argc; ++i) {
                context.emplace_back(argv[i]);
            }
        } else {
            context.resize(n-1, "<s>");
        }

        auto ng = ngram_predictor{n};
        std::cout << ng.predict_words(num_words_to_predict, context) << std::endl;

        ng.print_predicting_time();
    }
    else {
        std::cout<<"type should be 'train' or 'predict'";
    }
    report_memory_usage();
    return 0;
}