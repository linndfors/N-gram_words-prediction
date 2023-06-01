#include "ngram_predictor/reduce_n_gram.h"
#include "database/database.hpp"
#include "ngram_predictor.hpp"
#include <sqlite3.h>
#include <iostream>
#include <vector>

std::string generate_alter_query(const std::string& table_name, const std::string& col_name, const std::string& type) {
    std::string alter_table_sql = "ALTER TABLE " + table_name + " ADD COLUMN " + col_name + type;
    return alter_table_sql;
}

void reduce(std::string const& table_name, int n) {
    //...initialize database...
    sqlite3 *db;
    char *error_message = nullptr;
    int rc;


    //...connect or open database...
    rc = sqlite3_open(ngram_predictor::DB_PATH, &db);
    if(rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    //...all necessary queries...
    std::string table_name_reduced = "n" + std::to_string(n-1) + "_grams_frequency";
    std::string drop_table_sql = "DROP TABLE IF EXISTS " + table_name_reduced + ";";
    std::string create_table_sql = "CREATE TABLE IF NOT EXISTS " + table_name_reduced + "(ID_WORD_0 INT);";
    std::string select_frequency_sql = "SELECT FREQUENCY FROM " + table_name_reduced + " WHERE ";
    std::string general_select_sql = "SELECT * FROM " + table_name + ";";
    std::string update_frequency_sql = "UPDATE " + table_name_reduced + " SET FREQUENCY = ? WHERE ";

    for (int i = 0; i < n-1; ++i) {
        if (i > 0) {
            select_frequency_sql += " AND ";
            update_frequency_sql += " AND ";
        }
        select_frequency_sql += "ID_WORD_" + std::to_string(i) + " = ?";
        update_frequency_sql += "ID_WORD_" + std::to_string(i) + " = ?";
    }

    //...if reduced table exists - drop it...
    rc = sqlite3_exec(db, drop_table_sql.c_str(), nullptr, nullptr, &error_message);
    if (rc != SQLITE_OK) {
        std::cerr << "Problem with dropping table: " << error_message << std::endl;
        sqlite3_close(db);
        return;
    }

    //...create reduced table...
    rc = sqlite3_exec(db, create_table_sql.c_str(), nullptr, nullptr, &error_message);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << error_message << std::endl;
        sqlite3_close(db);
        return;
    }

    //...add columns...
    std::string alter_table_sql;
    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, &error_message);
    for (int i = 1; i < n-1; ++i) {
        std::string col_name = "ID_WORD_" + std::to_string(i);
        alter_table_sql = generate_alter_query(table_name_reduced, col_name, " INT");
        rc = sqlite3_exec(db, alter_table_sql.c_str(), nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK) {
            std::cerr<< "SQL error: " << error_message << std::endl;
            sqlite3_close(db);
            return;
        }
    }
    sqlite3_exec(db, "COMMIT", nullptr, nullptr, &error_message);

    //...add frequency column...
    alter_table_sql = generate_alter_query(table_name_reduced, "FREQUENCY", " INT DEFAULT 0");
    sqlite3_exec(db, alter_table_sql.c_str(), nullptr, nullptr, &error_message);

    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, general_select_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    //...prepare list of column names and values to bind...
    std::string col_names = "ID_WORD_0";
    std::string col_values = "?";
    for (int i = 1; i < n-1; ++i) {
        col_names += ", ID_WORD_" + std::to_string(i);
        col_values += ", ?";
    }
    std::string insert_reduced_table_sql = "INSERT INTO " + table_name_reduced + " (" + col_names + ", FREQUENCY) VALUES (" + col_values + ", ?)";

    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, &error_message);
    while (sqlite3_step(stmt) != SQLITE_DONE) {
        //...setting flag to indicate the start and end of the sentence...
        //...if n-gram contains n-1 tags <s> that means that this is the start of the sentence...
        //...the same for the end of the sentence...
        bool redundant_tags_start = true;
        bool redundant_tags_end = true;

        std::vector<int> row_id_values;
        for (int i = 0; i < n; ++i) {
            int id_word = sqlite3_column_int(stmt, i);
            if (id_word != 1 && i > 0) redundant_tags_start = false;
            if (id_word != 2 && i > 0) redundant_tags_end = false;
            row_id_values.emplace_back(id_word);
        }
        int frequency = sqlite3_column_int(stmt, n);

        if ((redundant_tags_end || redundant_tags_start) && n-1 < 2) {
            redundant_tags_start = redundant_tags_end = false;
        }

        if (!(redundant_tags_start || redundant_tags_end)) {
            //...standard ngram reduction...
            row_id_values.erase(row_id_values.begin());
            sqlite3_stmt* stmt;
            rc = sqlite3_prepare_v2(db, select_frequency_sql.c_str(), -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_close(db);
                return;
            }

            for (int i = 0; i < row_id_values.size(); ++i) {
                sqlite3_bind_int(stmt, i + 1, row_id_values[i]);
            }

            rc = sqlite3_step(stmt);

            //...checking whether extracted n-1gram already exists in a new table...
            if (rc == SQLITE_ROW) {
                int curr_freq = sqlite3_column_int(stmt, 0);
                frequency += curr_freq;
                sqlite3_stmt* update_stmt;
                rc = sqlite3_prepare_v2(db, update_frequency_sql.c_str(), -1, &update_stmt, nullptr);
                if (rc != SQLITE_OK) {
                    std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
                    sqlite3_close(db);
                    return;
                }

                sqlite3_bind_int(update_stmt, 1, frequency);
                for (int i = 0; i < n-1; ++i) {
                    int id_word = row_id_values[i];
                    sqlite3_bind_int(update_stmt, i+2, id_word);
                }

                rc = sqlite3_step(update_stmt);
                if (rc != SQLITE_DONE) {
                    std::cerr << "Error executing statement: " << sqlite3_errmsg(db) << std::endl;
                }

                sqlite3_finalize(update_stmt);
            } else if (rc == SQLITE_DONE) {
                sqlite3_stmt* curr_stmt;
                rc = sqlite3_prepare_v2(db, insert_reduced_table_sql.c_str(), -1, &curr_stmt, nullptr);
                if (rc != SQLITE_OK) {
                    std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
                    sqlite3_close(db);
                    return;
                }

                row_id_values.emplace_back(frequency);
                for (int i = 0; i < n; ++i) {
                    sqlite3_bind_int(curr_stmt, i+1, row_id_values[i]);
                }

                rc = sqlite3_step(curr_stmt);
                if (rc != SQLITE_DONE) {
                    std::cerr << "Error executing statement: " << sqlite3_errmsg(db) << std::endl;
                }

                sqlite3_finalize(curr_stmt);
            } else {
                std::cerr << "Error executing query: " << sqlite3_errmsg(db) << std::endl;
            }

            sqlite3_finalize(stmt);
        }
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, &error_message);

    //...close database...
    sqlite3_finalize(stmt);

    if (n == 2) {
        std::string increment_frequency_query = "UPDATE n1_grams_frequency SET FREQUENCY = FREQUENCY + 1 WHERE ID_WORD_0 = 1;";
        rc = sqlite3_exec(db, increment_frequency_query.c_str(), nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK) {
            std::cerr << "Update failed: " << error_message << std::endl;
            sqlite3_free(error_message);
            sqlite3_close(db);
            return;
        }
    }

    sqlite3_close(db);
}
