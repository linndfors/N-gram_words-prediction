#include "database.hpp"

#include <sqlite3.h>

#include <cassert>
#include <iostream>

DataBase::~DataBase()
{
    close();
}

auto DataBase::open(const std::string& path) -> bool
{
    if (auto result = sqlite3_open(path.c_str(), &m_dataBase); result != 0) {
        report_error();
        close();
    }

    return m_dataBase != nullptr;
}

auto DataBase::close() -> void
{
    if (m_dataBase != nullptr) {
        sqlite3_close(m_dataBase);
    }
}

auto DataBase::create_table(const std::string &name, const std::string &columns) -> void
{
    auto query = "CREATE TABLE" + name + "(" + columns + ");";
    execute_query(query);
}

auto DataBase::begin_transaction() -> void
{
    execute_query("BEGIN TRANSACTION;");
}

auto DataBase::commit() -> void
{
    execute_query("COMMIT;");
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
        report_error();
    }
}

auto DataBase::report_error() const -> void
{
    assert(m_dataBase != nullptr);
    std::cerr << "[ERROR]: " << sqlite3_errmsg(m_dataBase) << std::endl;
}
