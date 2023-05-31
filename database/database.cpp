#include "database.hpp"

#include <cassert>
#include <iostream>

DataBase::DataBase(const std::string& path)
{
    open(path);
}

DataBase::~DataBase()
{
    close();
}

auto DataBase::open(const std::string& path) -> void
{
    if (auto result = sqlite3_open(path.c_str(), &m_dataBase); result != 0) {
        report_error("Error opening database");
        close();
    }
}

auto DataBase::close() -> void
{
    if (m_dataBase != nullptr) {
        sqlite3_close(m_dataBase);
    }
}

auto DataBase::create_table(const std::string &name, const std::string &columns) -> void
{
    auto query = "CREATE TABLE " + name + " (" + columns + ");";
    execute_query(query);
}

auto DataBase::add_column(const std::string &table, const std::string &column) -> void
{
    auto query = "ALTER TABLE " + table + " ADD COLUMN " + column + ";";
    execute_query(query);
}

auto DataBase::drop_table(const std::string &name) -> void
{
    auto query = "DROP TABLE IF EXISTS " + name + ";";
    execute_query(query);
}

auto DataBase::begin_transaction() -> void
{
    execute_query("BEGIN TRANSACTION;");
}

auto DataBase::commit_transaction() -> void
{
    execute_query("COMMIT;");
}

auto DataBase::insert(const std::string &table, const std::string &columns, const std::string &values) -> void
{
    sqlite3_stmt* stmt;
    auto query = "INSERT INTO " + table + " (" + columns + ") VALUES(" + values + ");";
    if (sqlite3_prepare( m_dataBase, query.c_str(), -1, &stmt, nullptr ) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        report_error("Error preparing SQL query " + query);
    }
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        report_error("Error inserting data into table " + table);
    }
    sqlite3_finalize(stmt);
}


auto DataBase::execute_query(const std::string &query) -> void
{
    const auto callback = [](void *, int argc, char *argv[], char *columnName[]) -> int {
        for (auto i = 0; i < argc; ++i) {
            std::cout << columnName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
        }

        return 0;
    };

    if (auto result = sqlite3_exec(m_dataBase, query.c_str(), callback, nullptr, nullptr); result != SQLITE_OK) {
        report_error("Error executing SQL query " + query);
    }
}

auto DataBase::report_error(const std::string& msg) -> void
{
    assert(m_dataBase != nullptr);
    auto error = msg + "\n" + sqlite3_errmsg(m_dataBase);
    throw database_error(error);
}
