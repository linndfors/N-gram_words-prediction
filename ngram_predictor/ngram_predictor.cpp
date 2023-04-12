#include "ngram_predictor.hpp"


void ngram::print_list(const ngram::ngram_t &words)  {
    for (const auto& word : words) {
        std::cout << word << " ";
    }
    std::cout<<"\n";
}

void ngram::read_corpus(const std::string &file_name) {
    std::ifstream file(file_name);

    std::string word;
    ngram_t words;
    words.resize(n-1, "<s>");

    while (file >> word) {
        // Convert word to lowercase
        std::transform(word.begin(), word.end(), word.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        // Remove any non-alphanumeric characters from the word
        word.erase(std::remove_if(word.begin(), word.end(), [](char c) {
            return !std::isalnum(c); }), word.end());

        // Append word to current n-gram
        words.push_back(word);

        if (words.size() == n) {
            // If n-gram has been formed, add it to the map and remove the first word to shift the n-gram
            ngram_dict[words]++;
            words.erase(words.begin());
        }
    }
}

auto ngram::predict_word(const ngram_t& context) -> std::string {
    if (ngram_dict.empty() || context.empty() || context.size() < n-1){
        // No n-grams have been generated or the context is too short to generate a prediction
        return "";
    }

    std::string next_word;
    unsigned int max_count = 0;

    for (const auto& [ngram, freq] : ngram_dict) {

        if (std::equal(std::prev(context.end(), n-1), context.end(),
                       ngram.begin(), std::prev(ngram.end()))) {
//                for (auto elem:ngram) {
//                    std::cout<<elem<<",";
//                }
//                std::cout<<freq<<std::endl;
            if (freq > max_count) {
                next_word = ngram.back();
                max_count = freq;
            }
        }
    }
    return next_word;
}

auto ngram::predict_words(int num_words, ngram_t& context) -> ngram_t {
    if (ngram_dict.empty() || context.empty() || context.size() < n-1){
        // No n-grams have been generated or the context is too short to generate a prediction
        return {};
    }
//        std::cout<<predict_word;
//        std::string new_word = predict_word(context);
//        context.push_back(new_word);
    // predict new word based on n previous and add it to context
    for(int i = 0; i < num_words; ++i) {
        std::string predicted_word = predict_word(context);
        context.push_back(predicted_word);
    }
    return context;
}