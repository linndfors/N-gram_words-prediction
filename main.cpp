#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

typedef std::unordered_map<std::vector<std::string>, int>  dict;
typedef std::vector<std::string> word_list;


// to allow std::unordered_map use std::vector<std::string> as a key
namespace std {
    template<> struct hash<vector<string>> {
        size_t operator()(const vector<string>& v) const {
            size_t h = 0;
            for (const auto& s : v) {
                h ^= hash<string>{}(s) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
    };
}

class ngram {
private:
    int n;
    dict ngram_dict;
public:
    void read_corpus(const std::string &file_name) {
        std::ifstream file(file_name);

        std::string word;
        word_list words;
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

    ngram(const std::string& path, int n) : n(n) {
        read_corpus(path);
    }

    auto predict_word(const word_list& context) -> std::string {
        if (ngram_dict.empty() || context.empty() || context.size() < n-1){
            // No n-grams have been generated or the context is too short to generate a prediction
            return "";
        }

        std::string next_word;
        unsigned int max_count = 0;

        for (const auto& [ngram, freq] : ngram_dict) {
            if (std::equal(std::prev(context.end(), n-1), context.end(),
                           ngram.begin(), std::prev(ngram.end()))) {
                if (freq > max_count) {
                    next_word = ngram.back();
                    max_count = freq;
                }
            }
        }
        return next_word;
    }

    auto predict_words(int num_words, const word_list& context) -> word_list{
        if (ngram_dict.empty() || context.empty() || context.size() < n-1){
            // No n-grams have been generated or the context is too short to generate a prediction
            return {};
        }

        word_list predicted_words = {predict_word(context)};
        for(int i = 0; i < num_words; ++i) {
            std::string predicted_word = predict_word(predicted_words);
            predicted_words.push_back(predicted_word);
        }
        return predicted_words;
    }

    static void print_list(const word_list& words) {
        for (const auto& word : words) {
            std::cout << word << " ";
        }
    }
};

int main(int argc, char* argv[]) {
    std::string path;
    int n, num_words_to_predict;
    word_list context;

    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " path n num_words_to_predict context*" << std::endl;
        return 1;
    } else {
        path = argv[1];
        n = std::stoi(argv[2]);
        num_words_to_predict = std::stoi(argv[3]);

        if (argc > 4) {
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
