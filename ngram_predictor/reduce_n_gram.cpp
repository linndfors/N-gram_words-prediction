#include "reduce_n_gram.h"

ngram_predictor::ngram_dict_int_t reduce(ngram_predictor::ngram_dict_int_t ngram_map) {
    ngram_predictor::ngram_dict_int_t reduced_map;
    for (const auto& [ngram, freq] : ngram_map) {
        if (ngram[0] == 1) {
            ngram_predictor::ngram_id reduced_ngram;
            for (int i = 0; i < ngram.size()-1; ++i) {
                reduced_ngram.emplace_back(ngram[i]);
            }
            reduced_map[reduced_ngram] += freq;
            reduced_ngram.clear();
        }
        ngram_predictor::ngram_id reduced_ngram;
        for (int i = 1; i < ngram.size(); ++i) {
            reduced_ngram.emplace_back(ngram[i]);
        }
        reduced_map[reduced_ngram] += freq;
    }
    return reduced_map;
}

//int main() {
//    // just an example to show that it works correctly
//
//    std::string sentence = "<s> the quick brown fox jumps over the lazy dog and over the lazy cat </s> </s>";
//
//    // create a map of trigrams from the sentence
//    std::vector<std::string> words;
//    std::string word;
//    for (char c : sentence) {
//        if (c == ' ' || c == '\t' || c == '\n') {
//            if (!word.empty()) {
//                words.push_back(word);
//                word.clear();
//            }
//        } else {
//            word.push_back(c);
//        }
//    }
//    if (!word.empty()) {
//        words.push_back(word);
//    }
//    ngram_predictor::ngram_dict_t trigrams;
//    for (std::size_t i = 0; i < words.size() - 2; i++) {
//        std::vector<std::string> trigram { words[i], words[i+1], words[i+2] };
//        trigrams[trigram]++;
//    }
//
//    //print out trigrams
//    for (const auto& [key, value] : trigrams) {
//        std::cout << "{";
//        for (const auto& word : key) {
//            std::cout << word << " ";
//        }
//        std::cout << "}: " << value << std::endl;
//    }
//
//    std::cout << "---------------" << std::endl;
//
//    //create and print bigrams
//    ngram_predictor::ngram_dict_t bigrams = reduce(trigrams);
//    for (const auto& [key, value] : bigrams) {
//        std::cout << "{";
//        for (const auto& word : key) {
//            std::cout << word << " ";
//        }
//        std::cout << "}: " << value << std::endl;
//    }
//
//    std::cout << "---------------" << std::endl;
//
//    //create and print unigrams
//    ngram_predictor::ngram_dict_t unigrams = reduce(bigrams);
//    for (const auto& [key, value] : unigrams) {
//        std::cout << "{";
//        for (const auto& word : key) {
//            std::cout << word << " ";
//        }
//        std::cout << "}: " << value << std::endl;
//    }
//
//    return 0;
//}