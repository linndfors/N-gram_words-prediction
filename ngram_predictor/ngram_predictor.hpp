#ifndef NGRAM_NGRAM_PREDICTOR_HPP
#define NGRAM_NGRAM_PREDICTOR_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <boost/functional/hash.hpp>
#include "oneapi/tbb/concurrent_hash_map.h"


// to allow std::unordered_map use std::vector<typename T> as a key
template <typename T>
struct std::hash<std::vector<T>> {
    std::size_t operator()(std::vector<T> const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};


static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for(i = 0; i<argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}


class ngram_predictor {
public:
    using word = std::string;
    using id = uint32_t;
    using ngram_id = std::vector<id>;
    using ngram_str = std::vector<word>;
    using ngram_dict_t = std::unordered_map<word, id>;
    using ngram_dict_id_t = std::unordered_map<ngram_id, uint32_t>;
    // tbb
    using ngram_dict_id_tbb = oneapi::tbb::concurrent_hash_map<ngram_id, uint32_t>;
    using words_dict_tbb = oneapi::tbb::concurrent_hash_map<word, id>;

    ngram_predictor(std::string& path, int n);

    static void print_list(const std::vector<word>& words);

    void read_corpus();

    void print_training_time() const;
    void print_predicting_time() const;
    void print_writing_words_time() const;
    void write_ngrams_freq(const std::string& filename);
    void write_words_id(const std::string& filename);

    auto predict_words(int num_words, ngram_str& context) -> ngram_str;
    auto predict_words(int num_words, std::string& context) -> ngram_str;
private:
    int m_n;
    ngram_dict_id_tbb m_ngram_dict_id;

    uint32_t M_START_TAG_ID = 1;
    uint32_t M_END_TAG_ID = 2;
    uint32_t M_UNKNOWN_TAG_ID = 3;
    words_dict_tbb m_words_dict{{"<s>", M_START_TAG_ID}, {"</s>", M_END_TAG_ID}, {"<unk>", M_UNKNOWN_TAG_ID}};

    std::string m_path;
    std::unordered_set<std::string> m_indexing_extensions{".txt"};
    std::unordered_set<std::string> m_archives_extensions{".zip"};

    size_t m_max_live_tokens = 16;
    size_t m_max_file_size = 10000000;

    long long m_total_training_time{}, m_finding_time{}, m_reading_time{},
            m_writing_ngrams_to_db_time{}, m_writing_words_to_db_time{}, m_predicting_time{};

    static void check_if_path_is_dir(std::string &path);
    static auto read_file_into_binary(const std::filesystem::path& filename) -> std::pair<std::string, std::string>;

    auto get_path_if_fit(const std::filesystem::directory_entry& entry) -> std::filesystem::path;
    void count_ngrams_in_file(std::pair<std::string, std::string> file_content);
    void count_ngrams_in_archive(const std::string &archive_content);
    void count_ngrams_in_str(std::string &file_content);

    void parallel_read_pipeline();
    void write_ngrams_to_db();
    void write_words_to_db();

    auto convert_to_words(const ngram_id& ngram) -> ngram_str;
    auto convert_to_word(const id& id) -> word;
    auto convert_to_ids(const ngram_str& ngram, bool train) -> ngram_id;
    auto convert_to_id(const word& word, bool train) -> id;

    auto predict_id(const ngram_id& context) -> id;
};

#endif //NGRAM_NGRAM_PREDICTOR_HPP
