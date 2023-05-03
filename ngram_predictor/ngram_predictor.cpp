#include "ngram_predictor.hpp"
#include <boost/locale.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string>


void ngram_predictor::print_list(std::vector<word> words)  {
    for (const auto& word : words) {
        std::cout << word << " ";
    }
    std::cout<<"\n";
}

auto ngram_predictor::predict_id(const ngram_id& context) -> id {
    if (words_dict.empty() || context.empty() || context.size() < n-1){
        // No n-grams have been generated or the context is too short to generate a prediction
        return -1;
    }

//    std::string next_word;
    id next_id = -1;
    unsigned int max_count = 0;
    for (const auto& [ngram, freq] : ngram_dict_int) {

        if (std::equal(std::prev(context.end(), n-1), context.end(),
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
    if (words_dict.empty() || context.empty() || context.size() < n-1){
        // No n-grams have been generated or the context is too short to generate a prediction
        return {};
    }

    for (auto& word: context) {
        word = boost::locale::fold_case(boost::locale::normalize(word));
    }
    // predict new word based on n previous and add it to context
    for(int i = 0; i < num_words; ++i) {
        int predicted_id = predict_id(convert_to_ids(context, false));
        context.push_back(convert_to_word(predicted_id));
    }

    sqlite3 *db;
    char *zErrMsg = nullptr;
    int rc;

    rc = sqlite3_open("n_grams.db", &db);
    if (rc) {
        std::cerr<<"Problem with open database"<<std::endl;
        exit(6);
    }
    std::string sql = "DROP TABLE IF EXISTS all_words_id;";
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
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
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    for (const auto& pair : words_dict) {
        sqlite3_stmt* stmt;
        sql = "INSERT INTO all_words_id (ID, WORD) VALUES (?, ?)";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc == SQLITE_OK) {
            int id = pair.second;
            std::string word = pair.first;

            sqlite3_bind_int(stmt, 1, id);
            sqlite3_bind_text(stmt, 2, word.c_str(), -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Problem with inserting: " << sqlite3_errmsg(db) << std::endl;
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
    rc = sqlite3_exec(db, "COMMIT;", callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sqlite3_close(db);
//    std::cout<<"context: "<<context<<std::endl;
    return context;
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
        words_dict.insert(a, word);
        // if training, add it to the dictionary
        // if predicting, add <unk> to the dictionary
        if (a->second == 0) {
            if (train)
                a->second = static_cast<int>(words_dict.size());
            else
                a->second = static_cast<int>(convert_to_id("<unk>", true));
        }
        ngram_ids.push_back(a->second);
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
    for (const auto& [word, word_id] : words_dict) {
        if (word_id == id) {
            return word;
        }
    }
    return "";

}