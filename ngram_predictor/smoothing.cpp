#include <sqlite3.h>
#include <cstdlib>
#include "ngram_predictor/smoothing.h"
#include "ngram_predictor/ngram_predictor.hpp"
#include "ngram_predictor/reduce_n_gram.h"

double smoothing(ngram_predictor::ngram_id& ngram, double d) {
    int current_n = ngram.size();
    std::string current_table_name = "n" + std::to_string(current_n) + "_grams_frequency";
//    std::cout<<"start"<<std::endl
//    ;
    sqlite3* db;
    char* zErrMsg = nullptr;
    int rc;
    int frequency = 0;
    rc = sqlite3_open(ngram_predictor::DB_PATH, &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        exit(6);
    }

    sqlite3_stmt *stmt;
    std::string sql = "SELECT FREQUENCY FROM " + current_table_name + " WHERE ";
    for (int i = 0; i < current_n; ++i) {
        if (i > 0) sql += " AND ";
        sql += "ID_WORD_" + std::to_string(i) + " = ?";
    }
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    for (int i = 0; i < current_n; ++i) {
        sqlite3_bind_int(stmt, i + 1, ngram[i]);
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        frequency = sqlite3_column_int(stmt, 0);
//        std::cout<<"frequency: "<<frequency<<std::endl;
    }
    // все гуд


    double first_term_nom = std::max(frequency-d, 0.0);
    double first_term_denom = 0;
    double possible_next_words = 0; //counter for (n-1gram *)
    ngram_predictor::id final_word = ngram[current_n - 1]; //last word in given ngram
    double final_word_count = 0;

    sql = "SELECT SUM(FREQUENCY) AS total_sum, COUNT(*) AS row_count FROM " + current_table_name + " WHERE ";
    for (int i = 0; i < current_n-1; ++i) {
        if (i > 0) {
            sql += " AND ";
        }
        sql += "ID_WORD_" + std::to_string(i) + "= ?";
    }
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    for (int i = 0; i < current_n-1; ++i) {
        sqlite3_bind_int(stmt, i + 1, ngram[i]);
    }
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << std::endl;
//        sqlite3_close(db);
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        first_term_denom = sqlite3_column_int(stmt, 0);
        possible_next_words = sqlite3_column_int(stmt, 1);

        // Use the retrieved values as needed
//        std::cout << "Total Frequency Sum: " << totalSum << std::endl;
//        std::cout << "Row Count where n-1 appear: " << rowCount << std::endl;
    } else {
        std::cerr << "No results found." << std::endl;
//        sqlite3_close(db);
    }

    sqlite3_finalize(stmt);

    // все гуд
    sql = "SELECT COUNT(*) FROM " + current_table_name + " WHERE ID_WORD_" + std::to_string(current_n - 1)  + "=?;";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << std::endl;
//        sqlite3_close(db);
    }

// Bind the specific value to the query parameter
    std::string specificValue = std::to_string(ngram[current_n - 1]);
    sqlite3_bind_text(stmt, 1, specificValue.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        final_word_count = sqlite3_column_int(stmt, 0);

        // Use the retrieved value as needed
//        std::cout << "Row Count where last word in: " << rowCount << std::endl;
    } else {
        std::cerr << "No results found." << std::endl;
    }

    sqlite3_finalize(stmt);
//    std::cout<<"fnom: "<<first_term_nom<<std::endl;
//    std::cout<<"fdenom: "<<first_term_denom<<std::endl;
    double first_term = first_term_nom/first_term_denom;

    double lambda = d / (first_term_denom * possible_next_words);

    sql = "SELECT COUNT(*) FROM " + current_table_name + ";";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << std::endl;
//        sqlite3_close(db);
    }

    rc = sqlite3_step(stmt);
    int table_size = 0;
    if (rc == SQLITE_ROW) {
        table_size = sqlite3_column_int(stmt, 0);

        // Use the retrieved value as needed
//        std::cout << "Total Row Count: " << table_size << std::endl;
    } else {
        std::cerr << "No results found." << std::endl;
    }

    sqlite3_finalize(stmt);

    double p_count = final_word_count/table_size;
//    std::cout<<"p count: "<<p_count<<std::endl;
//    std::cout<<"first term: "<<first_term<<std::endl;
//    std::cout<<"lambda: "<<lambda<<std::endl;
    double prob = first_term + (lambda * p_count);
//    std::cout<<prob<<std::endl;
    if (prob != 0) {
        sqlite3_close(db);
        return prob;
    }
    else {
        std::cout<<"reducing here"<<std::endl;
//        reduce(current_table_name, current_n - 1);
        ngram.erase(ngram.begin());
        return smoothing(ngram, 0.5);
    }
}




//double old_smoothing(ngram::ngram_dict_t ngrams_dict, ngram::ngram_t ngram, double d) {
//
//    int frequency = ngrams_dict[ngram];
//    double first_term_nom = std::max(frequency-d, 0.0);
//
//    double first_term_denom = 0;
//    double possible_next_words = 0; //counter for (n-1gram *)
//    std::string final_word = ngram[ngram.size()-1]; //last word in given ngram
//    double final_word_count = 0;
//
//    for (const auto& [curr_ngram, freq] : ngrams_dict) {
//
//        size_t reduced_ngram_size = curr_ngram.size()-1;
//        for (int i = 0; i < reduced_ngram_size; ++i) {
//            if (curr_ngram[i] != ngram[i]) break;
//            if (i == reduced_ngram_size-1) {
//                first_term_denom += freq;
//                ++possible_next_words;
//            }
//        }
//
//        //check if the last word is in current ngram
//        if (final_word == curr_ngram[curr_ngram.size() - 1]) {
//            ++final_word_count;
//        }
//    }
//
//    double first_term = first_term_nom/first_term_denom;
//
//    double lambda = d / (first_term_denom * possible_next_words);
//
//    double p_count = final_word_count/ngrams_dict.size();
//
//    double prob = first_term + lambda * p_count;
//
//    // check if in the corpus there is no such ngram, then do smoothing for reduced ngram
//    if (prob) return prob;
//    else {
//        ngram.erase(ngram.begin());
//        return smoothing(ngrams_dict, ngram, 0.5);
//    }
//}
