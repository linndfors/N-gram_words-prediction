#include "db_manager.hpp"
#include <iostream>

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for(i = 0; i < argc; i++) {
        auto temp_out = argv[i] ? argv[i] : "NULL";
        std::cout << azColName[i] << " = " << temp_out << std::endl;
    }
    return 0;
}

db_manager::db_manager(const std::string &path) {
    rc = sqlite3_open(path.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(6);
    }
}

db_manager::~db_manager() {
    sqlite3_close(db);
}

void db_manager::create_table(const std::string &table_name, const std::string &columns) {
    auto sql = "CREATE TABLE" + table_name + "(" + columns + ");";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ) {
        std::cerr<<"Problem with creating table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }
}

void db_manager::begin_transaction() {
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        exit(6);
    }
}

void db_manager::commit() {
    rc = sqlite3_exec(db, "COMMIT;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        exit(6);
    }
}