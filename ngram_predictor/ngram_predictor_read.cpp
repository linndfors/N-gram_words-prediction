#include <limits>
#include <boost/locale.hpp>
#include <filesystem>
#include <thread>
#include <archive.h>
#include <archive_entry.h>
#include "time_measurements.hpp"
#include "ngram_predictor.hpp"

ngram_predictor::ngram_predictor(std::string& path, int n) : n(n), path(path) {
    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");
    std::locale::global(loc);
};


void ngram_predictor::read_corpus() {
    auto start = get_current_time_fenced();

    check_if_path_is_dir();
    auto path_iter = std::filesystem::recursive_directory_iterator(path);
    auto path_end = std::filesystem::end(path_iter);

    oneapi::tbb::parallel_pipeline(max_live_tokens,
                                   oneapi::tbb::make_filter<void, std::filesystem::path>(
                                           oneapi::tbb::filter_mode::serial_out_of_order,
                                           [&](oneapi::tbb::flow_control& fc) -> std::filesystem::path {
                                               auto start = get_current_time_fenced();
                                               auto temp = find_files(fc, path_iter, path_end);
                                               auto end = get_current_time_fenced();
                                               finding_time += to_ms(end - start);
                                               return temp;
                                           }) &
                                   oneapi::tbb::make_filter<std::filesystem::path, std::pair<std::string, std::string>>(
                                           oneapi::tbb::filter_mode::serial_out_of_order,
                                           [this](const std::filesystem::path &filename) -> std::pair<std::string, std::string> {
                                               auto start = get_current_time_fenced();
                                               auto temp = read_files_into_binaries(filename);
                                               auto end = get_current_time_fenced();
                                               reading_time += to_ms(end - start);
                                               return temp;
                                           }) &
                                   oneapi::tbb::make_filter<std::pair<std::string, std::string>, void>(
                                           oneapi::tbb::filter_mode::parallel,
                                           [this](const std::pair<std::string, std::string>& raw_file) {
                                               count_ngrams(raw_file);
                                           })
    );

    auto finish = get_current_time_fenced();
    total_time = to_ms(finish - start);
}

void ngram_predictor::check_if_path_is_dir() {
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: input directory " << path << " does not exist." << std::endl;
        exit(26);
    }
    if (!std::filesystem::is_directory(path)) {
        std::cerr << "Error: expected directory, got " << path << std::endl;
        exit(26);
    }
}

std::filesystem::path ngram_predictor::find_files(oneapi::tbb::flow_control& fc, std::filesystem::recursive_directory_iterator& iter,
                                                  std::filesystem::recursive_directory_iterator& end) {

    if (iter == end) {
        fc.stop();
        return {};
    }

    auto entry = *(iter++);

    std::string extension = entry.path().extension().string();
    if ( (indexing_extensions.find(extension) != indexing_extensions.end() && entry.file_size() < max_file_size) ||
         (archives_extensions.find(extension) != archives_extensions.end()) ) {
        return entry.path();
    }

    return {};
}

std::pair<std::string, std::string> ngram_predictor::read_files_into_binaries(const std::filesystem::path& filename) {
    if (filename.empty()) {
        return {};
    }
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
//            std::cerr << "Failed to open file " << filename << std::endl;
        return {};
    }

    // "really large files" solution from https://web.archive.org/web/20180314195042/http://cpp.indi.frih.net/blog/2014/09/how-to-read-an-entire-file-into-memory-in-cpp/
    auto const start_pos = file.tellg();
    file.ignore(std::numeric_limits<std::streamsize>::max());
    auto const char_count = file.gcount();
    file.seekg(start_pos);
    auto s = std::string(char_count, char{});
    file.read(&s[0], static_cast<std::streamsize>(s.size()));
    file.close();

    return std::pair{s, filename.extension().string()};
}

void ngram_predictor::count_ngrams(std::pair<std::string, std::string> file_content) {
    if (file_content.first.empty() || file_content.second.empty()) {
        return;
    }

    if (archives_extensions.find(file_content.second) != archives_extensions.end()) {
        read_archive(file_content.first);
    } else {
        count_ngrams_in_str(file_content.first);
    }
}

void ngram_predictor::read_archive(const std::string &file_content) {
    struct archive *archive = archive_read_new();
    struct archive_entry *entry;

    archive_read_support_format_all(archive);
    archive_read_support_filter_all(archive);

    int r = archive_read_open_memory(archive, file_content.data(), file_content.size());
    if (r != ARCHIVE_OK) {
        return;
    }

    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        auto extension = std::filesystem::path(archive_entry_pathname(entry)).extension().string();
        if (indexing_extensions.find(extension) == indexing_extensions.end()) {
            continue;
        }
        size_t size = archive_entry_size(entry);
        if (size <= 0 || size > max_file_size) {
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

    auto contents = bl::fold_case(bl::normalize(file_content));
    bl::boundary::ssegment_index words_index(bl::boundary::word, contents.begin(), contents.end());
    words_index.rule(bl::boundary::word_letters);

    ngram_t temp_ngram;
    ngram_dict_t temp_ngram_dict;
    auto it = words_index.begin();
    auto e = words_index.end();

    int i = 0;
    for (; i < n && it != e; ++i, ++it) {
        temp_ngram.emplace_back(*it);
    }
    // if word count < n, fill with <s>
    for (; i < n; ++i) {
        temp_ngram.insert(temp_ngram.begin(), "<s>");
    }
    // access to ngram_dict is thread-safe
    ngram_dict_t_tbb::accessor a;
    ngram_dict.insert(a, temp_ngram);
    ++a->second;
    for (; it != e; ++it) {
        temp_ngram.erase(temp_ngram.begin());
        temp_ngram.emplace_back(*it);
        ngram_dict.insert(a, temp_ngram);
        ++a->second;
    }
}


void ngram_predictor::print_time() const {
    std::cout << "Total counting time: " << total_time << " ms" << std::endl;
    std::cout << "Finding files time: " << finding_time << " ms" << std::endl;
    std::cout << "Reading files time: " << reading_time << " ms" << std::endl;
}

void ngram_predictor::write_ngrams_count(const std::string &filename) {
    std::ofstream out_file(filename);
    if (!out_file.is_open()) {
        std::cerr << "Error: failed to open file for writing: " << filename << std::endl;
        exit(4);
    }

    try {
        for (auto const &[key, val]: ngram_dict) {
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

// easier to debug on big files, but slower
void ngram_predictor::write_ngrams_count_with_sort(const std::string &filename) {
    std::ofstream out_file(filename);
    if (!out_file.is_open()) {
        std::cerr << "Error: failed to open file for writing: " << filename << std::endl;
        exit(4);
    }

    std::vector<std::pair<std::vector<std::string>, int>> sorted_words(ngram_dict.begin(), ngram_dict.end());
    std::sort(sorted_words.begin(), sorted_words.end(),
              [](const std::pair<std::vector<std::string>, int> &a,
                 const std::pair<std::vector<std::string>, int> &b) {
                  return a.second > b.second;
              });

    try {
        for (auto const &[key, val]: sorted_words) {
            for (auto const &word: key) {
                out_file << word << " ";
            }
            out_file << "   " << val << std::endl;
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Error writing in an out file: " << filename << std::endl;
        exit(6);
    }

    out_file.close();
}
