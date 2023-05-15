#ifndef NGRAM_DB_MANAGER_HPP
#define NGRAM_DB_MANAGER_HPP

#include <cstdlib>
#include <sqlite3.h>
#include <string>


class db_manager {
public:
    db_manager(const std::string& path);
    db_manager(const db_manager&) = delete;
    db_manager& operator=(const db_manager&) = delete;
    db_manager(db_manager&&) = delete;
    ~db_manager();

    void create_table(const std::string& table_name, const std::string& columns);
    void insert(const std::string& table_name, const std::string& columns, const std::string& values);
    void select(const std::string& table_name, const std::string& columns, const std::string& where);
    void update(const std::string& table_name, const std::string& columns, const std::string& where);
    void delete_(const std::string& table_name, const std::string& where);
    void add_column(const std::string& table_name, const std::string& column_name, const std::string& type);

    void commit();
    void begin_transaction();

    int execute_query(const std::string& query);


private:
    sqlite3 *db;
    char *zErrMsg = nullptr;
    int rc;
    sqlite3_stmt* stmt;
};


#endif //NGRAM_DB_MANAGER_HPP
