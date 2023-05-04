#include "reduce_n_gram.h"

void reduce(std::string const& table_name, int n) {
    sqlite3 *db;
    char *error_message = 0;
    int rc;
    rc = sqlite3_open("../n_grams.db", &db);

    if(rc) {
        std::cout << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    } else {
        std::cout << "Opened database successfully" << std::endl;
    }

    std::string table_name_reduced = "n" + std::to_string(n-1) + "_grams_frequency";
    std::string sql = "DROP TABLE IF EXISTS " + table_name_reduced + ";";
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &error_message);
    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with dropping table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sql = "CREATE TABLE IF NOT EXISTS " + table_name_reduced + "(ID_WORD_0 INT);";
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &error_message);
    if( rc != SQLITE_OK ){
        std::cout << "SQL error: " << error_message << std::endl;
        sqlite3_free(error_message);
    } else {
        std::cout << "Table created successfully" << std::endl;
    }
    sqlite3_exec(db, "BEGIN TRANSACTION", callback, NULL, &error_message);
    for (int i = 1; i < n-1; ++i) {
        std::string col_name = "ID_WORD_" + std::to_string(i);
        std::string sql = "ALTER TABLE " + table_name_reduced + " ADD COLUMN " + col_name + " INT";
        rc = sqlite3_exec(db, sql.c_str(), callback, 0, &error_message);
        if (rc != SQLITE_OK) {
            std::cout << "SQL error: " << error_message << std::endl;
            sqlite3_free(error_message);
        } else {
            std::cout << "Added column successfully" << std::endl;
        }
    }
    sqlite3_exec(db, "COMMIT", callback, NULL, &error_message);
    sql = "ALTER TABLE " + table_name_reduced + " ADD COLUMN " + "FREQUENCY" + " INT DEFAULT 0";
    sqlite3_exec(db, sql.c_str(), callback, 0, &error_message);

    sqlite3_stmt* stmt;
    sql = "SELECT * FROM " + table_name;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

    std::string col_names = "ID_WORD_0";
    std::string col_values = "?";
    for (int i = 1; i < n-1; ++i) {
        col_names += ", ID_WORD_" + std::to_string(i);
        col_values += ", ?";
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        std::vector<int> row_id_values;
        int first_id = sqlite3_column_int(stmt, 0);
        if (first_id == 100) {
            for (int i = 0; i < n-1; ++i) {
                int id_word = sqlite3_column_int(stmt, i);
                row_id_values.push_back(id_word);
            }
            int frequency = sqlite3_column_int(stmt, n);
            sql = "INSERT INTO " + table_name_reduced + " (" + col_names + ", FREQUENCY) VALUES (" + col_values + ", ?)";
            sqlite3_stmt* curr_stmt;
            sqlite3_prepare_v2(db, sql.c_str(), -1, &curr_stmt, NULL);

            row_id_values.emplace_back(frequency);
            for (int i = 0; i < n; ++i) {
                sqlite3_bind_int(curr_stmt, i+1, row_id_values[i]);
            }

            sqlite3_step(curr_stmt);
            sqlite3_finalize(curr_stmt);
            row_id_values.clear();
        }
        for (int i = 1; i <= n-1; ++i) {
            int id_word = sqlite3_column_int(stmt, i);
            row_id_values.push_back(id_word);
        }
        int frequency = sqlite3_column_int(stmt, n);
//        row_id_values.push_back(frequency);
        std::string sql = "SELECT FREQUENCY FROM " + table_name_reduced + " WHERE ";
        for (int i = 0; i < n-1; ++i) {
            if (i > 0) sql += " AND ";
            sql += "ID_WORD_" + std::to_string(i) + " = ?";
        }
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
        for (int i = 0; i < row_id_values.size(); ++i) {
            sqlite3_bind_int(stmt, i + 1, row_id_values[i]);
        }
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            int freq = sqlite3_column_int(stmt, 0);
            std::cout << "The frequency is: " << freq << std::endl;
            frequency += freq;
            std::string update_sql = "UPDATE " + table_name_reduced + " SET FREQUENCY = ? WHERE ";
            for (int i = 0; i < n-1; ++i) {
                if (i > 0) update_sql += " AND ";
                update_sql += "ID_WORD_" + std::to_string(i) + " = ?";
            }
            sqlite3_stmt* update_stmt;
            sqlite3_prepare_v2(db, update_sql.c_str(), -1, &update_stmt, NULL);
            sqlite3_bind_int(update_stmt, 1, frequency);
            for (int i = 0; i < n-1; ++i) {
                int id_word = row_id_values[i];
                sqlite3_bind_int(update_stmt, i+2, id_word);
            }
            sqlite3_step(update_stmt);
            sqlite3_finalize(update_stmt);
        } else if (rc == SQLITE_DONE) {
            std::cout << "No matching row found" << std::endl;
            sql = "INSERT INTO " + table_name_reduced + " (" + col_names + ", FREQUENCY) VALUES (" + col_values + ", ?)";
            sqlite3_stmt* curr_stmt;
            sqlite3_prepare_v2(db, sql.c_str(), -1, &curr_stmt, NULL);

            row_id_values.emplace_back(frequency);
            for (int i = 0; i < n; ++i) {
                sqlite3_bind_int(curr_stmt, i+1, row_id_values[i]);
            }

            sqlite3_step(curr_stmt);
            sqlite3_finalize(curr_stmt);
        } else {
            std::cerr << "Error executing query: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_finalize(stmt);
    }
    std::cout << "Database is closed" << std::endl;

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

//int main() {
//    reduce("n2_grams_frequency", 2);
//    return 0;
//}