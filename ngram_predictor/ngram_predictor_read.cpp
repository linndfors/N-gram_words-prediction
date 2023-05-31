#include "ngram_predictor/ngram_predictor.hpp"
#include "ngram_predictor/time_measurements.hpp"

#include "database/exception.hpp"
#include <thread>
#include <fstream>
#include <boost/locale.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <oneapi/tbb/parallel_pipeline.h>
#include <iostream>

void ngram_predictor::read_corpus(const std::string& path)
{
    check_if_path_is_dir(path);

    auto start = get_current_time_fenced();

    parallel_read_pipeline(path);
    try {
        write_words_to_db();
        write_ngrams_to_db();
    }
    catch (const database_error& e) {
        std::cerr << e.what() << std::endl;
        e.exit();
    }

    auto finish = get_current_time_fenced();
    m_total_training_time = to_ms(finish - start);
}

void ngram_predictor::parallel_read_pipeline(const std::string &path) {
    auto path_iter = std::filesystem::recursive_directory_iterator(path);
    auto path_end = std::filesystem::end(path_iter);

    std::vector<std::thread> threads;
    for (int i = 0; i < MERGE_THREADS; ++i) {
        threads.emplace_back([&]() { merge_temp_ngram_dict_to_global();});
    }

    m_accessors = 1;
    oneapi::tbb::parallel_pipeline(MAX_LIVE_TOKENS,
                   oneapi::tbb::make_filter<void, std::filesystem::path>(
                   oneapi::tbb::filter_mode::serial_out_of_order,
                   [&](oneapi::tbb::flow_control& fc) -> std::filesystem::path {
                       auto start = get_current_time_fenced();

                       if (path_iter == path_end) {
                           fc.stop();
                           return {};
                       }
                       auto entry = *(path_iter++);
                       auto temp = get_path_if_fit(entry);

                       auto end = get_current_time_fenced();
                       m_finding_time += to_ms(end - start);
                       return temp;
                   }) &
                    oneapi::tbb::make_filter<std::filesystem::path, std::pair<std::string, std::string>>(
                   oneapi::tbb::filter_mode::serial_out_of_order,
                   [this](const std::filesystem::path &filename) -> std::pair<std::string, std::string> {
                       auto start = get_current_time_fenced();

                       auto temp = read_file_into_binary(filename);

                       auto end = get_current_time_fenced();
                       m_reading_time += to_ms(end - start);
                       return temp;
                   }) &
                    oneapi::tbb::make_filter<std::pair<std::string, std::string>, void>(
                   oneapi::tbb::filter_mode::parallel,
                   [this](const std::pair<std::string, std::string>& raw_file) {
                       count_ngrams_in_file(raw_file);
                   })
    );
    m_accessors = 0;
    m_merge_cv.notify_all();

    for (auto& thread : threads) {
        thread.join();
    }

}

auto ngram_predictor::get_path_if_fit(const std::filesystem::directory_entry& entry) -> std::filesystem::path {
    auto extension = entry.path().extension().string();
    if ((m_indexing_extensions.find(extension) != m_indexing_extensions.end() && entry.file_size() < MAX_FILE_SIZE) ||
        (m_archives_extensions.find(extension) != m_archives_extensions.end()) ) {
        return entry.path();
    }
    return {};
}

auto ngram_predictor::read_file_into_binary(const std::filesystem::path& filename) -> std::pair<std::string, std::string> {
    if (filename.empty()) {
        return {{},{}};
    }
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
//        std::cerr << "Failed to open file " << filename << std::endl;
        return {{},{}};
    }

    auto char_count = std::filesystem::file_size(filename);
    auto s = std::string(char_count, char{});
    file.read(&s[0], static_cast<std::streamsize>(s.size()));
    file.close();

    return std::pair{s, filename.extension().string()};
}

void ngram_predictor::count_ngrams_in_file(std::pair<std::string, std::string> file_content) {
    if (file_content.first.empty() || file_content.second.empty()) {
        return;
    }

    if (m_archives_extensions.find(file_content.second) != m_archives_extensions.end()) {
        count_ngrams_in_archive(file_content.first);
    } else {
        count_ngrams_in_str(file_content.first);
    }
}

void ngram_predictor::count_ngrams_in_archive(const std::string &archive_content) {
    struct archive *archive = archive_read_new();
    struct archive_entry *entry;

    archive_read_support_format_all(archive);
    archive_read_support_filter_all(archive);

    int r = archive_read_open_memory(archive, archive_content.data(), archive_content.size());
    if (r != ARCHIVE_OK) {
        return;
    }

    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        auto extension = std::filesystem::path(archive_entry_pathname(entry)).extension().string();
        if (m_indexing_extensions.find(extension) == m_indexing_extensions.end()) {
            continue;
        }
        size_t size = archive_entry_size(entry);
        if (size <= 0 || size > MAX_FILE_SIZE) {
            continue;
        }
        std::string contents;
        contents.resize(size);
        long long k = archive_read_data(archive, &contents[0], size);
        if (k <= 0) {
            continue;
        }

        count_ngrams_in_str(contents);
    }

    archive_read_free(archive);
}


void ngram_predictor::count_ngrams_in_str(std::string &file_content) {
    namespace bl = boost::locale;

    // Normalize and fold case and, then, split into sentences
    auto contents = bl::fold_case(bl::normalize(file_content));
    bl::boundary::ssegment_index sentence_index(bl::boundary::sentence, contents.begin(), contents.end());
    sentence_index.rule(bl::boundary::sentence_term);

    ngram_dict_id curr_ngram_dict;

    for (const auto &sentence : sentence_index) {
        // Split into words
        bl::boundary::ssegment_index words_index(bl::boundary::word, sentence.begin(), sentence.end());
        words_index.rule(bl::boundary::word_letters);

        // Skip empty sentences
        if (words_index.begin() == words_index.end()) {
            continue;
        }

        // Add <s> tag at the beginning of the sentence
        ngram_id temp_ngram;
        for (int i = 0; i < m_n - 1; ++i) {
            temp_ngram.emplace_back(START_TAG_ID);
        }

        for (const auto &word : words_index) {
            if (word.empty()) {
                continue;
            }
            temp_ngram.emplace_back(convert_to_id(word, true));
            if (temp_ngram.size() == m_n) {
//                m_ngram_dict_id.insert(a, temp_ngram);
//                ++a->second;
                curr_ngram_dict[temp_ngram]++;
                temp_ngram.erase(temp_ngram.begin());
            }
        }

        // Add </s> tags at the end of the sentence
        for (int i = 0; i < (m_n - 1); ++i) {
            temp_ngram.emplace_back(END_TAG_ID);
            if (temp_ngram.size() == m_n) {
//                m_ngram_dict_id.insert(a, temp_ngram);
//                ++a->second;
                curr_ngram_dict[temp_ngram]++;
                temp_ngram.erase(temp_ngram.begin());
            }
        }
    }

    if (curr_ngram_dict.empty()) {
        return;
    }

    std::cout << "waiting to push " << std::this_thread::get_id()<< std::endl;
    m_q_temp_ngram_dict.push(curr_ngram_dict);
    std::cout << "pushed " << std::this_thread::get_id()<< std::endl;
    m_merge_cv.notify_one();
}

void ngram_predictor::merge_temp_ngram_dict_to_global() {
    std::cout << "Merge thread started" << std::endl;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_merge_mutex);
            ngram_dict_id curr_ngram_dict;
            std::cout << std::this_thread::get_id() << " merge started waiting" << std::endl;
            m_merge_cv.wait(lock,
                            [this] {
                return !m_q_temp_ngram_dict.empty() || (!m_accessors && m_was_writen_to_db);
            });
            std::cout << std::this_thread::get_id() << " merge finished waiting" << std::endl;
            if (m_q_temp_ngram_dict.empty()) {
                std::cout << std::this_thread::get_id() << " merge I leave" << std::endl;
                break;
            }
            m_q_temp_ngram_dict.pop(curr_ngram_dict);
            for (auto const &[key, val]: curr_ngram_dict) {
                m_ngram_dict_id[key] += val;
            }
            std::cout << std::this_thread::get_id() << " merge I added" << std::endl;
        }
        {
            std::unique_lock<std::mutex> lock(m_merge_mutex);
            std::cout << "GET NGRAM DICT SIZE " << m_ngram_dict_id.size() << std::endl;
            if (m_ngram_dict_id.size() > MAX_NGRAM_DICT_SIZE || (!m_accessors && !m_was_writen_to_db)) {
                std::cout << std::this_thread::get_id() << " merge adding to db" << std::endl;
                write_ngrams_to_db();
                m_ngram_dict_id.clear();
            }
        }
    }
    std::cout << "Merge thread finished" << std::this_thread::get_id() << std::endl;
}

void ngram_predictor::write_ngrams_freq(const std::string &filename) {
    std::ofstream out_file(filename);
    if (!out_file.is_open()) {
        std::cerr << "Error: failed to open file for writing: " << filename << std::endl;
        exit(4);
    }

    try {
        for (auto const &[key, val]: m_ngram_dict_id) {
            for (auto const &word: key) {
                out_file << word << " ";
            }
            out_file << "   " << val << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error writing in an out file: " << filename << std::endl;
        exit(6);
    }

    out_file.close();
}

void ngram_predictor::write_words_id(const std::string &filename) {
    std::ofstream out_words_file(filename);
    if (!out_words_file.is_open()) {
        std::cerr << "Error: failed to open file for writing: " << filename << std::endl;
        exit(4);
    }

    try {
        for (auto const &[key, val]: m_words_dict) {
            out_words_file << key << "   " << val << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error writing in an out file: " << filename << std::endl;
        exit(6);
    }

    out_words_file.close();
}
