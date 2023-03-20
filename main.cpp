#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <cmath>

std::vector<std::string> read_corpus(const std::string& file_name) {
    std::ifstream file(file_name);
    std::string word;
    std::vector<std::string> words;
    while (file >> word) {
        // Convert word to lowercase
        std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c){ return std::tolower(c); });
        // Remove any non-alphanumeric characters from the word
        word.erase(std::remove_if(word.begin(), word.end(), [](char c){ return !std::isalnum(c); }), word.end());
        words.push_back(word);
    }
    return words;
}

// combines ngram in one string
std::vector<std::string> generate_ngrams(int n, const std::vector<std::string>& words) {
    std::vector<std::string> ngrams;
    for (int i = 0; i < words.size() - n + 1; i++) {
        std::string ngram;
        for (int j = 0; j < n; j++) {
            if (j > 0) {
                ngram += " ";
            }
            ngram += words[i + j];
        }
        ngrams.push_back(ngram);
    }
    return ngrams;
}

std::unordered_map<std::string, int> get_ngram_frequencies(const std::vector<std::string>& ngrams) {
    std::unordered_map<std::string, int> frequency_map;
    for (const auto& ngram : ngrams) {
        ++frequency_map[ngram];
    }
    return frequency_map;
}

long double calculate_log_prob(const std::string& ngram, const std::unordered_map<std::string, int>& frequency_map,
                               const std::unordered_map<std::string, int>& n1gram_map) {

    if (n1gram_map.find(ngram.substr(0, ngram.find_last_of(' '))) == n1gram_map.end()) {
        return log(1.0 / static_cast<long>(n1gram_map.size() + 1)); // unseen n-1 gram
    }

    return log((double)(frequency_map.find(ngram)->second
    + 1)/static_cast<long>(n1gram_map.find(ngram.substr(0, ngram.find_last_of(' ')))->second
    + n1gram_map.size() + 1));
}


std::map<std::vector<std::string>, unsigned int> generate_ngram(const std::vector<std::string>& words, unsigned int n) {
    std::map<std::vector<std::string>, unsigned int> ngram_dict;
    if (words.size() < n) {
        // Input vector is too short to generate n-grams of size n.
        return ngram_dict;
    }
    for (unsigned int i = 0; i <= words.size() - n; ++i) {
        std::vector<std::string> ngram(words.begin() + i, words.begin() + i + n);
        ngram_dict[ngram]++;
    }
    return ngram_dict;
}


std::string predict_next_word(const std::map<std::vector<std::string>, unsigned int>& ngram_dict,
                              const std::vector<std::string>& prev_words) {
    unsigned int n = ngram_dict.begin()->first.size();
    if (ngram_dict.empty() || prev_words.empty() || prev_words.size() < n-1){
        return "";
    }

    std::vector<std::string> prev_words_n1(std::prev(prev_words.end(), n-1), prev_words.end());

    // Create a map of all possible next words and their frequencies
    std::map<std::string, unsigned int> next_words;
    for (const auto & iter : ngram_dict) {
        const auto& ngram = iter.first;
        const auto& freq = iter.second;
        if (std::equal(prev_words_n1.begin(), prev_words_n1.end(), ngram.begin(), ngram.end() - 1)) {
            next_words[ngram.back()] += freq;
        }
    }

    // Find the most frequent next word
    std::string next_word;
    unsigned int max_count = 0;
    for (const auto & iter : next_words) {
        const auto& word = iter.first;
        const auto& freq = iter.second;
        if (freq > max_count) {
            next_word = word;
            max_count = freq;
        }
    }
    return next_word;
}

std::vector<std::string> predict_words(int num_words, const std::vector<std::string>& prev_words, std::map<std::vector<std::string>, unsigned int>& ngrams) {
    std::vector<std::string> predicted_words = {predict_next_word(ngrams, prev_words)};
    for(int i=0; i<num_words; ++i) {
        std::string predicted_word = predict_next_word(ngrams, predicted_words);
        predicted_words.push_back(predicted_word);
    }
    return predicted_words;
}

void print_list(const std::vector<std::string>& words) {
    for (const auto& word : words) {
        std::cout << word << " ";
    }
}


int main() {
    std::vector<std::string> words = read_corpus("corpus.txt");

//    std::unordered_map<std::string, int> unigrams_freq = get_ngram_frequencies(words);

//    std::unordered_map<std::string, int> bigrams_freq = get_ngram_frequencies(bigrams);
//    for (const auto& bigram : bigrams_freq) {
//        std::cout << bigram.first <<" "<< bigram.second << std::endl;
//    }

    std::map<std::vector<std::string>, unsigned int> bigram_vec =  generate_ngram(words, 2);
    std::vector<std::string> predicted_words = predict_words(10, {"sun"}, bigram_vec);
    print_list(predicted_words);

    return 0;
}
