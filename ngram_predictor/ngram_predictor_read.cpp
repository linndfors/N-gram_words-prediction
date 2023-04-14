#include <limits>
#include <boost/locale.hpp>
#include <filesystem>
#include <thread>
#include <archive.h>
#include <archive_entry.h>
#include "ngram_predictor.hpp"

ngram::ngram(std::string& path, int n) : n(n), path(path),
filenames_queue(filenames_queue_size),
raw_files_queue(raw_files_queue_size) {
    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");
    std::locale::global(loc);
};


void ngram::read_corpus() {
    std::vector<std::thread> threads;
    threads.emplace_back(&ngram::find_files, this);
    threads.emplace_back(&ngram::read_files_into_binaries, this);
    for (size_t i = 0; i < indexing_threads; ++i) {
        threads.emplace_back(&ngram::count_ngrams, this);
    }

    for (auto &thread: threads) {
        thread.join();
    }
}

void ngram::find_files() {
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: input directory " << path << " does not exist." << std::endl;
        exit(26);
    }
    if (!std::filesystem::is_directory(path)) {
        std::cerr << "Error: expected directory, got " << path << std::endl;
        exit(26);
    }

    for (const auto &entry: std::filesystem::recursive_directory_iterator(path)) {
        std::string extension = entry.path().extension().string();
        if ((std::find(indexing_extensions.begin(), indexing_extensions.end(), extension) != indexing_extensions.end())
            && (entry.file_size() < max_file_size) && (entry.file_size() > 0)) {
            filenames_queue.push(const_cast<std::filesystem::path &&>(entry.path()));
        } else if (std::find(archives_extensions.begin(), archives_extensions.end(), extension) !=
                   archives_extensions.end()) {
            filenames_queue.push(const_cast<std::filesystem::path &&>(entry.path()));
        }
    }
    filenames_queue.push(std::filesystem::path{}); // end of queue
}

void ngram::read_files_into_binaries() {
    while (true) {
        std::filesystem::path filename = filenames_queue.pop();
        if (filename.empty()) {
            for (size_t i = 0; i < indexing_threads; ++i) {
                raw_files_queue.push(std::move(std::pair<std::string, std::string>{})); // end of queue
            }
            break;
        }
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
//            std::cerr << "Failed to open file " << filename << std::endl;
            continue;
        }

        // "really large files" solution from https://web.archive.org/web/20180314195042/http://cpp.indi.frih.net/blog/2014/09/how-to-read-an-entire-file-into-memory-in-cpp/
        auto const start_pos = file.tellg();
        file.ignore(std::numeric_limits<std::streamsize>::max());
        auto const char_count = file.gcount();
        file.seekg(start_pos);
        auto s = std::string(char_count, char{});
        file.read(&s[0], static_cast<std::streamsize>(s.size()));
        file.close();

        if (!s.empty()) {
            raw_files_queue.push(std::move(std::pair<std::string, std::string>{s, filename.extension().string()}));
        }
    }
}

void ngram::count_ngrams() {
    while (true) {
        std::pair<std::string, std::string> file_content = raw_files_queue.pop();
        if (file_content.first.empty() || file_content.second.empty()) {
            break;
        }

        if (std::find(archives_extensions.begin(), archives_extensions.end(), file_content.second) !=
            archives_extensions.end()) {

            read_archive(file_content.first);
        } else {
            count_ngrams_in_str(file_content.first);
        }
    }
}

void ngram::read_archive(const std::string &file_content) {
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
        if (std::find(indexing_extensions.begin(), indexing_extensions.end(), extension) ==
            indexing_extensions.end()) {
            continue;
        }
        size_t size = archive_entry_size(entry);
        if (size > max_file_size || size <= 0) {
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

void ngram::count_ngrams_in_str(std::string &file_content) {
    namespace bl = boost::locale;

    file_content = bl::fold_case(bl::normalize(file_content));
    bl::boundary::ssegment_index map(bl::boundary::word, file_content.begin(),file_content.end());
    map.rule(bl::boundary::word_letters);

    ngram_t temp_ngram;
    auto it = map.begin();
    auto e = map.end();

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


void ngram::write_ngrams_count(const std::string &filename) {
    std::ofstream out_file(filename);
    if (!out_file.is_open()) {
        std::cerr << "Error: failed to open file for writing: " << filename << std::endl;
        exit(4);
    }

    for (auto const &[key, val]: ngram_dict) {
        for (auto const &word: key) {
            out_file << word << " ";
        }
        out_file << "   " << val << std::endl;
        if (out_file.bad()) {
            std::cerr << "Error writing in an out file: " << filename << std::endl;
            exit(6);
        }
    }

    out_file.close();
}

// easier to debug on big files, but slower
void ngram::write_ngrams_count_with_sort(const std::string &filename) {
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

    for (auto const &[key, val]: sorted_words) {
        for (auto const &word: key) {
            out_file << word << " ";
        }
        out_file << "   " << val << std::endl;
        if (out_file.bad()) {
            std::cerr << "Error writing in an out file: " << filename << std::endl;
            exit(6);
        }
    }

    out_file.close();
}
