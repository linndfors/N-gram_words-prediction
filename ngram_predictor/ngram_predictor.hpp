#ifndef NGRAM_NGRAM_PREDICTOR_HPP
#define NGRAM_NGRAM_PREDICTOR_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <boost/functional/hash.hpp>


// to allow std::unordered_map use std::vector<std::string> as a key
template <>
struct std::hash<std::vector<std::string>> {
    std::size_t operator()(std::vector<std::string> const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};


class ngram {
public:
    using ngram_t = std::vector<std::string>;
    using ngram_dict_t = std::unordered_map<ngram_t, int>;

    ngram(const std::string& path, int n) : n(n) {
        read_corpus(path);
    }

    static void print_list(const ngram_t& words);

    void read_corpus(const std::string &file_name);

    auto predict_word(const ngram_t& context) -> std::string;

    auto predict_words(int num_words, ngram_t& context) -> ngram_t;

private:
    int n;
    ngram_dict_t ngram_dict;
};

#endif //NGRAM_NGRAM_PREDICTOR_HPP
