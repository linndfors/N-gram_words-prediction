#ifndef NGRAM_NGRAM_PREDICTOR_HPP
#define NGRAM_NGRAM_PREDICTOR_HPP

#include <boost/locale.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <mutex>
#include <condition_variable>
#include <boost/functional/hash.hpp>
#include <oneapi/tbb/concurrent_hash_map.h>
#include <oneapi/tbb/concurrent_queue.h>

template <typename T>
struct std::hash<std::vector<T>> 
{
    std::size_t operator()(std::vector<T> const& c) const 
    {
        return boost::hash_range(c.begin(), c.end());
    }
};

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for(i = 0; i < argc; i++) {
        auto temp_out = argv[i] ? argv[i] : "NULL";
        std::cout << azColName[i] << " = " << temp_out << std::endl;
    }
    return 0;
}

class ngram_predictor 
{
public:
    using word = std::string;
    using ngrams = std::vector<word>;
    using id = uint32_t;
    using ngram_id = std::vector<id>;
    using ngram_dict_id = std::unordered_map<ngram_id, uint32_t>;
    using q_ngram_dict_id = oneapi::tbb::concurrent_bounded_queue<ngram_dict_id>;
    using ngram_dict_id_tbb = oneapi::tbb::concurrent_hash_map<ngram_id, uint32_t>;
    using words_dict_tbb = oneapi::tbb::concurrent_hash_map<word, id>;
    static constexpr auto DB_PATH = "./ngrams_test_bd.db";


    explicit ngram_predictor(int n);
    ngram_predictor(const ngram_predictor&) = delete;
    ngram_predictor(ngram_predictor&&) = delete;
    auto operator=(const ngram_predictor&) -> ngram_predictor& = delete;
    auto operator=(ngram_predictor&&) -> ngram_predictor& = delete;
    ~ngram_predictor() = default;

    void read_corpus(const std::string& path);
    auto predict_words(int num_words, ngrams& context) -> ngrams;
    auto clean_context(ngrams& context) const -> ngrams;
    auto remove_tags(ngrams& words) -> ngrams;

    auto print_training_time() const -> void;
    auto print_predicting_time() const -> void;


    auto write_ngrams_freq(const std::string& filename) -> void;
    auto write_words_id(const std::string& filename) -> void;
    
private:
    static constexpr auto MAX_LIVE_TOKENS = size_t{16};
    static constexpr auto MERGE_THREADS = size_t{4};
    static constexpr auto MAX_NGRAM_DICT_SIZE = size_t{50'000'000};
    static constexpr auto MAX_Q_SIZE = size_t{1000};
    static constexpr auto MAX_FILE_SIZE = size_t{10'000'000};
    
    static constexpr auto START_TAG_ID = uint32_t{1};
    static constexpr auto END_TAG_ID = uint32_t{2};
    static constexpr auto UNKNOWN_TAG_ID = uint32_t{3};

    static auto check_if_path_is_dir(const std::string &path) -> void;
    static auto read_file_into_binary(const std::filesystem::path& filename) -> std::pair<std::string, std::string>;

    auto get_path_if_fit(const std::filesystem::directory_entry& entry) -> std::filesystem::path;
    auto count_ngrams_in_file(std::pair<std::string, std::string> file_content) -> void;
    auto count_ngrams_in_archive(const std::string &archive_content) -> void;
    auto count_ngrams_in_str(std::string &file_content) -> void;
    auto merge_temp_ngram_dict_to_global() -> void;

    auto parallel_read_pipeline(const std::string& path) -> void;
    auto write_ngrams_to_db() -> void;
    auto write_words_to_db() -> void;

    auto convert_to_words(const ngram_id& ngram) const -> ngrams;
    static auto convert_to_word(const id& id) -> word;
    auto convert_to_ids(const ngrams& ngram, bool train) -> ngram_id;
    auto convert_to_id(const word& word, bool train) -> id;

    auto predict_id(const ngram_id& context) const -> id;
    static auto find_word(int n, const ngram_id& context);
    int m_n;
    std::mutex m_words_id_mutex;
    std::mutex m_merge_mutex;
    std::condition_variable m_merge_cv;
    int m_accessors{1};
    bool m_was_writen_to_db{false};
    ngram_dict_id m_ngram_dict_id;
    words_dict_tbb m_words_dict;
    uint32_t m_last_word_id{UNKNOWN_TAG_ID};
    q_ngram_dict_id m_q_temp_ngram_dict;

    std::unordered_set<std::string> m_indexing_extensions{".txt"};
    std::unordered_set<std::string> m_archives_extensions{".zip"};

    size_t m_total_training_time{};
    size_t m_finding_time{};
    size_t m_reading_time{};
    size_t m_writing_ngrams_to_db_time{};
    size_t m_writing_words_to_db_time{};
    size_t m_predicting_time{};
};

#endif //NGRAM_NGRAM_PREDICTOR_HPP
