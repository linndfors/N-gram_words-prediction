#include "ngram_predictor/reduce_n_gram.h"

std::string generate_alter_query(std::string table_name, std::string col_name, std::string type) {
    std::string alter_table_sql = "ALTER TABLE " + table_name + " ADD COLUMN " + col_name + type;
    return alter_table_sql;
}

void reduce(std::string const& table_name, int n) {

    //...initialize database...
    sqlite3 *db;
    char *error_message = 0;
    int rc;

    //...all necessary queries...
    std::string table_name_reduced = "n" + std::to_string(n-1) + "_grams_frequency";
    std::string drop_table_sql = "DROP TABLE IF EXISTS " + table_name_reduced + ";";
    std::string create_table_sql = "CREATE TABLE IF NOT EXISTS " + table_name_reduced + "(ID_WORD_0 INT);";
    std::string select_frequency_sql = "SELECT FREQUENCY FROM " + table_name_reduced + " WHERE ";
    std::string general_select_sql = "SELECT * FROM " + table_name;
    std::string update_frequency_sql = "UPDATE " + table_name_reduced + " SET FREQUENCY = ? WHERE ";
    for (int i = 0; i < n-1; ++i) {
        if (i > 0) select_frequency_sql += " AND ";
        select_frequency_sql += "ID_WORD_" + std::to_string(i) + " = ?";
    }
    for (int i = 0; i < n-1; ++i) {
        if (i > 0) update_frequency_sql += " AND ";
        update_frequency_sql += "ID_WORD_" + std::to_string(i) + " = ?";
    }

    //...connect or open database...
    rc = sqlite3_open("../n_grams.db", &db);
    if(rc) {
        std::cout << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    //...if reduced table exists - drop it...
    rc = sqlite3_exec(db, drop_table_sql.c_str(), callback, 0, &error_message);
    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with dropping table"<<std::endl;
        sqlite3_close(db);
        return;
    }

    //...create reduced table...
    rc = sqlite3_exec(db, create_table_sql.c_str(), callback, 0, &error_message);
    if( rc != SQLITE_OK ){
        std::cout << "SQL error: " << error_message << std::endl;
        sqlite3_close(db);
        return;
    }

    //...add columns...
    std::string alter_table_sql;
    sqlite3_exec(db, "BEGIN TRANSACTION", callback, NULL, &error_message);
    for (int i = 1; i < n-1; ++i) {
        std::string col_name = "ID_WORD_" + std::to_string(i);
        alter_table_sql = generate_alter_query(table_name_reduced, col_name, " INT");
        rc = sqlite3_exec(db, alter_table_sql.c_str(), callback, 0, &error_message);
        if (rc != SQLITE_OK) {
            std::cout << "SQL error: " << error_message << std::endl;
            sqlite3_close(db);
            return;
        }
    }
    sqlite3_exec(db, "COMMIT", callback, NULL, &error_message);

    //...add frequency column...
    alter_table_sql = generate_alter_query(table_name_reduced, "FREQUENCY", " INT DEFAULT 0");
    sqlite3_exec(db, alter_table_sql.c_str(), callback, 0, &error_message);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, general_select_sql.c_str(), -1, &stmt, NULL);

    //...prepare list of column names and values to bind...
    std::string col_names = "ID_WORD_0";
    std::string col_values = "?";
    for (int i = 1; i < n-1; ++i) {
        col_names += ", ID_WORD_" + std::to_string(i);
        col_values += ", ?";
    }
    std::string insert_reduced_table_sql = "INSERT INTO " + table_name_reduced + " (" + col_names + ", FREQUENCY) VALUES (" + col_values + ", ?)";

    while (sqlite3_step(stmt) != SQLITE_DONE) {

        std::vector<int> row_id_values;
        for (int i = 0; i < n; ++i) {
            int id_word = sqlite3_column_int(stmt, i);
            row_id_values.emplace_back(id_word);
        }
        int first_id = sqlite3_column_int(stmt, 0);
        int frequency = sqlite3_column_int(stmt, n);
        //...check ngram whether it is the first one in the document..
        if (first_id == 100) {
            int last_id = row_id_values[n-1];
            row_id_values.pop_back();
            sqlite3_stmt* curr_stmt;
            sqlite3_prepare_v2(db, insert_reduced_table_sql.c_str(), -1, &curr_stmt, NULL);
            row_id_values.emplace_back(frequency);
            for (int i = 0; i < n; ++i) {
                sqlite3_bind_int(curr_stmt, i+1, row_id_values[i]);
            }
            sqlite3_step(curr_stmt);
            sqlite3_finalize(curr_stmt);
            row_id_values.pop_back();
            row_id_values.emplace_back(last_id);
        }

        //...standard ngram reduction...
        row_id_values.erase(row_id_values.begin());
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, select_frequency_sql.c_str(), -1, &stmt, NULL);
        for (int i = 0; i < row_id_values.size(); ++i) {
            sqlite3_bind_int(stmt, i + 1, row_id_values[i]);
        }
        int rc = sqlite3_step(stmt);

        //...checking whether extracted n-1gram already exists in a new table...
        if (rc == SQLITE_ROW) {
            int curr_freq = sqlite3_column_int(stmt, 0);
            frequency += curr_freq;
            sqlite3_stmt* update_stmt;
            sqlite3_prepare_v2(db, update_frequency_sql.c_str(), -1, &update_stmt, NULL);
            sqlite3_bind_int(update_stmt, 1, frequency);
            for (int i = 0; i < n-1; ++i) {
                int id_word = row_id_values[i];
                sqlite3_bind_int(update_stmt, i+2, id_word);
            }
            sqlite3_step(update_stmt);
            sqlite3_finalize(update_stmt);
        } else if (rc == SQLITE_DONE) {
            sqlite3_stmt* curr_stmt;
            sqlite3_prepare_v2(db, insert_reduced_table_sql.c_str(), -1, &curr_stmt, NULL);
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

    //...close database...
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

//int main() {
//    reduce("n2_grams_frequency", 2);
//    return 0;
//}