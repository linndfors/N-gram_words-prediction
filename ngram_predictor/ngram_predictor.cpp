#include "ngram_predictor.hpp"
#include "time_measurements.hpp"
#include <iostream>
#include <boost/locale.hpp>
#include <cstdlib>
#include <sqlite3.h>
#include <string>

ngram_predictor::ngram_predictor(std::string& path, int n) : m_n(n), m_path(path) {
    check_if_path_is_dir(path);

    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");
    std::locale::global(loc);
}

void ngram_predictor::check_if_path_is_dir(std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Error: input directory " << filename << " does not exist." << std::endl;
        exit(26);
    }
    if (!std::filesystem::is_directory(filename)) {
        std::cerr << "Error: expected directory, got " << filename << std::endl;
        exit(26);
    }
}

void ngram_predictor::print_list(const std::vector<word>& words)  {
    for (const auto& word : words) {
        std::cout << word << " ";
    }
    std::cout<<"\n";
}

auto ngram_predictor::predict_id(const ngram_id& context) -> id {
    if (m_words_dict.empty() || context.empty() || context.size() < m_n - 1){
        std::cerr << "Error: no n-grams have been generated or the context is too short to generate a prediction" << std::endl;
        return -1;
    }

    id next_id = -1;
    unsigned int max_count = 0;
    for (const auto& [ngram, freq] : m_ngram_dict_id) {

        if (std::equal(std::prev(context.end(), m_n - 1), context.end(),
                       ngram.begin(), std::prev(ngram.end()))) {
            if (freq > max_count) {
                next_id = ngram.back();
                max_count = freq;
            }
        }
    }
    return next_id;
}

auto ngram_predictor::predict_words(int num_words, ngram_str& context) -> ngram_str {
    auto start = get_current_time_fenced();

    if (m_words_dict.empty() || context.empty() || context.size() < m_n - 1){
        std::cerr << "Error: no n-grams have been generated or the context is too short to generate a prediction" << std::endl;
        return {};
    }

    for (auto& word: context) {
        word = boost::locale::fold_case(boost::locale::normalize(word));
    }

    auto context_ids = convert_to_ids(context, false);
    // predict new word based on n previous and add it to context
    for(int i = 0; i < num_words; ++i) {
        auto predicted_id = predict_id(context_ids);
        context_ids.push_back(predicted_id);
    }
    // get back words that was unknown to the model
    auto result = convert_to_words(context_ids);
    for (size_t i = 0; i < context.size(); ++i) {
        result[i] = context[i];
    }

    auto end = get_current_time_fenced();
    m_predicting_time = to_ms(end - start);
    write_words_to_db();

    return result;
}

auto ngram_predictor::predict_words(int num_words, std::string& context) -> ngram_str {
    namespace bl = boost::locale;
    auto contents = bl::fold_case(bl::normalize(context));
    bl::boundary::ssegment_index words_index(bl::boundary::word, contents.begin(), contents.end());
    words_index.rule(bl::boundary::word_letters);

    ngram_str vector_of_words;
    for (const auto& word: words_index) {
        vector_of_words.emplace_back(word);
    }

    return predict_words(num_words, vector_of_words);
}

auto ngram_predictor::convert_to_ids(const ngram_predictor::ngram_str &ngram, bool train) -> ngram_id {
    ngram_id ngram_ids;
    words_dict_tbb::accessor a;
    for (const auto& word : ngram) {
        // if predicting and word does not exist in m_words_dict, add <unk> to the dictionary
        if (!m_words_dict.find(a, word) && !train) {
            ngram_ids.push_back(M_UNKNOWN_TAG_ID);
            continue;
        }

        if (m_words_dict.find(a, word)) {
            ngram_ids.push_back(a->second);
        } else {
            m_words_dict.insert(a, word);
            a->second = m_words_dict.size();
            ngram_ids.push_back(a->second);
        }
    }
    return ngram_ids;
}

auto ngram_predictor::convert_to_id(const ngram_predictor::word &word, bool train) -> id {
    return convert_to_ids({word}, train).front();
}

auto ngram_predictor::convert_to_words(const ngram_predictor::ngram_id &ngram) -> ngram_str {
    ngram_str ngram_words;
    for (const auto& id : ngram) {
        ngram_words.push_back(convert_to_word(id));
    }
    return ngram_words;
}

auto ngram_predictor::convert_to_word(const ngram_predictor::id &id) -> word {
    for (const auto& [word, word_id] : m_words_dict) {
        if (word_id == id) {
            return word;
        }
    }
    return "";
}


void ngram_predictor::print_training_time() const {
    std::cout << "Total training time: " << m_total_training_time << " ms" << std::endl;
    std::cout << "  Finding files time: " << m_finding_time << " ms" << std::endl;
    std::cout << "  Reading files time: " << m_reading_time << " ms" << std::endl;
    std::cout << "  Counting ngrams time: " << m_total_training_time - m_writing_ngrams_to_db_time << " ms" << std::endl;
    std::cout << "  Writing ngrams to db time: " << m_writing_ngrams_to_db_time << " ms" << std::endl;
    std::cout << std::endl;
}

void ngram_predictor::print_predicting_time() const {
    std::cout << "Predicting time: " << m_predicting_time << " ms" << std::endl;
}

void ngram_predictor::print_writing_words_time() const {
    std::cout << "Writing words to db time: " << m_writing_words_to_db_time << " ms" << std::endl;
}


void ngram_predictor::write_ngrams_to_db() {
    auto start = get_current_time_fenced();

    sqlite3 *db;
    char *zErrMsg = nullptr;
    int rc;

    rc = sqlite3_open("n_grams.db", &db);

    if (rc) {
        std::cerr<<"Problem with opening database"<<std::endl;
        exit(6);
    }
    std::string table_name = "n" + std::to_string(m_n) + "_grams_frequency";
    std::string sql = "DROP TABLE IF EXISTS " + table_name + ";";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with droping table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sql = "CREATE TABLE " + table_name + "(ID_WORD_0 INT);";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with creating table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }

    if (m_n > 1) {
        for (int j = 1; j < m_n; ++j) {
            std::string col_name = "ID_WORD_" + std::to_string(j);
            std::string sql_add_col = "ALTER TABLE " + table_name + " ADD COLUMN " + col_name + " INT";
            rc = sqlite3_exec(db, sql_add_col.c_str(), callback, nullptr, &zErrMsg);
            if (rc != SQLITE_OK) {
                std::cerr<<"Problem with adding columns"<<std::endl;
                sqlite3_close(db);
                exit(6);
            }
        }
    }
    sql = "ALTER TABLE " + table_name + " ADD COLUMN " + "FREQUENCY" + " INT";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    for (const auto& pair : m_ngram_dict_id) {
        ngram_id words = pair.first;
        auto value = pair.second;
        std::string fields;
        std::string words_id;
        for (int i = 0; i < m_n; ++i) {
            words_id += std::to_string(words[i]);
            words_id += ", ";
            fields += "ID_WORD_" + std::to_string(i);
            fields += ", ";
        }
        sql = "INSERT INTO " + table_name + "(" + fields + "FREQUENCY) VALUES (" + words_id + std::to_string(value) + ");";

        rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
        if (rc != SQLITE_OK) {
            std::cerr<<"Problem with inserting"<<std::endl;
            sqlite3_close(db);
            exit(6);
        }
    }
    rc = sqlite3_exec(db, "COMMIT;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sqlite3_close(db);

    auto end = get_current_time_fenced();
    m_writing_ngrams_to_db_time = to_ms(end - start);
}

void ngram_predictor::write_words_to_db() {
    auto start = get_current_time_fenced();

    sqlite3 *db;
    char *zErrMsg = nullptr;
    int rc;

    rc = sqlite3_open("n_grams.db", &db);
    if (rc) {
        std::cerr<<"Problem with open database"<<std::endl;
        exit(6);
    }
    std::string sql = "DROP TABLE IF EXISTS all_words_id;";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with droping table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }

    sql = "CREATE TABLE all_words_id (ID INT PRIMARY KEY, WORD VARCHAR(50));";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ) {
        std::cerr<<"Problem with creating table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    for (const auto& pair : m_words_dict) {
        sqlite3_stmt* stmt;
        sql = "INSERT INTO all_words_id (ID, WORD) VALUES (?, ?)";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc == SQLITE_OK) {
            auto id = pair.second;
            std::string word = pair.first;

            sqlite3_bind_int(stmt, 1, static_cast<int>(id));
            sqlite3_bind_text(stmt, 2, word.c_str(), -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Problem with inserting: " << sqlite3_errmsg(db) << std::endl;
                std::cout << "id: " << id << " word: " << word << std::endl;
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                exit(6);
            }
            sqlite3_finalize(stmt);
        } else {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            exit(6);
        }
    }
    rc = sqlite3_exec(db, "COMMIT;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sqlite3_close(db);

    auto end = get_current_time_fenced();
    m_writing_words_to_db_time = to_ms(end - start);
}