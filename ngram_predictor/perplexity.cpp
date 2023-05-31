#include "perplexity.h"

double calculate_conditional_prob(ngram_predictor::ngram_id& vals, int n, DataBase& db) {

    sqlite3_stmt* stmt;
    int rc;
    double nom = 0;
    double denom = 0;

    std::string table_name = "n" + std::to_string(n) +"_grams_frequency";
    std::string sum_freq_query = "";

    if (n >= 2) {
        sum_freq_query = "SELECT SUM(FREQUENCY) FROM " + table_name + " WHERE ";

        for (int i = 0; i < n - 1; ++i) {
            if (i > 0) {
                sum_freq_query += " AND ";
            }
            sum_freq_query += "ID_WORD_" + std::to_string(i) + " = " + std::to_string(vals[i]);
        }
    } else {
        sum_freq_query = "SELECT SUM(FREQUENCY) FROM " + table_name;
    }

    rc = sqlite3_prepare_v2(db.m_dataBase, sum_freq_query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        std::cerr << "Error preparing query: " + sum_freq_query << std::endl;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        denom = sqlite3_column_int(stmt, 0);
    } else {
        std::cerr << "Error executing query" << std::endl;
    }

    if (n >= 2) {
        sum_freq_query += " AND ID_WORD_" + std::to_string(n-1) + " = " + std::to_string(vals[n-1]);
    }
    else {
        sum_freq_query += " WHERE ID_WORD_" + std::to_string(n-1) + " = " + std::to_string(vals[n-1]);
    }

    rc = sqlite3_prepare_v2(db.m_dataBase, sum_freq_query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        std::cerr << "Error preparing query: " + sum_freq_query << std::endl;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        nom = sqlite3_column_int(stmt, 0);
    } else {
        std::cerr << "Error executing query" << std::endl;
    }

    if (nom == 0) ++nom;
    if (denom == 0) ++denom;

    double conditional_prob = nom / denom;
    return conditional_prob;
}

double calculate_ppl(int n, const ngram_predictor::ngram_id& sentence) {

    auto db = DataBase(ngram_predictor::DB_PATH);

    ngram_predictor::ngram_id curr_ngram;

    for (int i = 0; i < n; ++i) {
        curr_ngram.emplace_back(sentence[i]);
    }

    double ppl = 0;

    for (int i = n; i < sentence.size(); ++i) {
        double conditional_prob = calculate_conditional_prob(curr_ngram, n, db);
        ppl += -std::log(conditional_prob);
        curr_ngram.erase(curr_ngram.begin());
        curr_ngram.emplace_back(sentence[i]);
    }

    double result = std::exp(ppl / sentence.size());
    return result;
}