#include "perplexity.h"

double calculate_conditional_prob(ngram_predictor::ngram_id& vals, int n, DataBase& db) {

    sqlite3_stmt* stmt;
    int rc;
    int nom = 0;
    int denom = 0;

    std::string table_name = "n" + std::to_string(n) +"_grams_frequency";
    std::string sum_freq_query = "SELECT SUM(FREQUENCY) FROM " + table_name + " WHERE ";

    for (int i = 0; i < n-1; ++i) {
        if (i > 0) {
            sum_freq_query += " AND ";
        }
        sum_freq_query += "ID_WORD_" + std::to_string(i) + " = " + std::to_string(vals[i]);
    }

    rc = sqlite3_prepare_v2(db.m_dataBase, sum_freq_query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        std::cerr << "Error preparing query: " + sum_freq_query << std::endl;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        denom = sqlite3_column_int(stmt, 0);
        std::cout << "denom: " << denom << std::endl;
    } else {
        std::cerr << "Error executing query" << std::endl;
    }

    sum_freq_query += " AND ID_WORD_" + std::to_string(n-1) + " = " + std::to_string(vals[n-1]);
    rc = sqlite3_prepare_v2(db.m_dataBase, sum_freq_query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        std::cerr << "Error preparing query: " + sum_freq_query << std::endl;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        nom = sqlite3_column_int(stmt, 0);
        std::cout << "nom: " << nom << std::endl;
    } else {
        std::cerr << "Error executing query" << std::endl;
    }

    double conditional_prob = log(nom / denom);
    return conditional_prob;
}

//...we need to pass the size of n-gram, the vector of ids that represent sentence, and the size of the context etc...
double calculate_ppl(int n, ngram_predictor::ngram_id sentence) {

    //...connect or open database...
    auto db = DataBase(ngram_predictor::DB_PATH);

    ngram_predictor::ngram_id curr_ngram;

    for (int i = 0; i < n; ++i) {
        curr_ngram.emplace_back(sentence[i]);
    }

    double ppl = 0;

    for (int i = n; i < sentence.size(); ++i) {
        double conditional_prob = calculate_conditional_prob(curr_ngram, n, db);
        ppl += 1 / conditional_prob;
        curr_ngram.erase(curr_ngram.begin());
        curr_ngram.emplace_back(sentence[i]);
    }

    double result = std::pow(ppl, 1.0 / n);
    return result;
}