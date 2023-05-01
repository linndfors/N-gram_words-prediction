#ifndef NGRAM_NGRAM_PREDICTOR_HPP
#define NGRAM_NGRAM_PREDICTOR_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <boost/functional/hash.hpp>
#include <filesystem>
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/parallel_pipeline.h"


// to allow std::unordered_map use std::vector<std::string> as a key
template <>
struct std::hash<std::vector<std::string>> {
    std::size_t operator()(std::vector<std::string> const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};

// to allow std::unordered_map use std::vector<int> as a key
template <>
struct std::hash<std::vector<int>> {
    std::size_t operator()(std::vector<int> const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};


class ngram_predictor {
public:
    using word = std::string;
    using id = int;
    using ngram_id = std::vector<id>;
    using ngram_str = std::vector<word>;
    using ngram_dict_t = std::unordered_map<word, id>;
    using ngram_dict_int_t = std::unordered_map<ngram_id, int>;

    // tbd
//    using ngram_t = std::vector<word_t>;
//    using ngram_dict_t = std::unordered_map<ngram_t, int>;
    using ngram_dict_t_tbb = oneapi::tbb::concurrent_hash_map<ngram_id, int>;
    using words_dict_tbb = oneapi::tbb::concurrent_hash_map<word, id>;

    ngram_predictor(std::string& path, int n);

    static void print_list(std::vector<word> words);

    void read_corpus();

    void print_time() const;
    void write_ngrams_count(const std::string& filename);
    //void write_ngrams_count_with_sort(const std::string& filename);

    auto predict_id(const ngram_id& context) -> id ;

    auto predict_words(int num_words, ngram_str& context) -> ngram_str;
    auto predict_words(int num_words, std::string& context) -> ngram_str;
//    auto predict_word(int num_words, ngram_str& context) -> word;
    auto convert_to_words(const ngram_id& ngram) -> ngram_str;
    auto convert_to_word(const id& id) -> word;
    auto convert_to_ids(const ngram_str& ngram) -> ngram_id;
    auto convert_to_id(const word& word) -> id;

private:
    int n;
//    ngram_dict_t ngram_dict;
    ngram_dict_t_tbb ngram_dict_int;

    // word - id
    words_dict_tbb words_dict{{"<s>", 1}};
    // [id, id] - count
//    ngram_dict_int_t ngram_dict_int;

    std::string path;
    std::unordered_set<std::string> indexing_extensions{".txt"};
    std::unordered_set<std::string> archives_extensions{".zip"};
    size_t max_live_tokens = 16;
    size_t max_file_size = 10000000;

    void check_if_path_is_dir();
    void read_archive(const std::string &file_content);
    void count_ngrams_in_str(std::string &file_content);

    std::filesystem::path find_files(oneapi::tbb::flow_control& fc, std::filesystem::recursive_directory_iterator& iter,
                                     std::filesystem::recursive_directory_iterator& end);
    std::pair<std::string, std::string> read_files_into_binaries(const std::filesystem::path& filename);
    void count_ngrams(std::pair<std::string, std::string> file_content);

    long long total_time{}, finding_time{}, reading_time{};
};

#endif //NGRAM_NGRAM_PREDICTOR_HPP
