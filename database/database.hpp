// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#pragma once

#include <string>
#include <stdexcept>
#include <sqlite3.h>
#include <vector>
#include "exception.hpp"

struct sqlite3;


class DataBase 
{
public:
    explicit DataBase(const std::string& path);
    DataBase(const DataBase&) = delete;
    DataBase(DataBase&&) = delete;    
    
    auto operator=(const DataBase&) -> DataBase& = delete;
    auto operator=(DataBase&&) -> DataBase& = delete;

    ~DataBase();

    auto open(const std::string& path) -> void;
    auto close() -> void;

    auto create_table(const std::string& name, const std::string& columns) -> void;
    auto add_column(const std::string& table, const std::string& column) -> void;
    auto drop_table(const std::string& name) -> void;
    auto begin_transaction() -> void;
    auto commit_transaction() -> void;
    auto pragma_shrink_memory_vacuum() -> void;

    auto create_unique_index(const std::string& table, const std::string& columns) -> void;
    auto insert(const std::string& table, const std::string& columns, const std::string& values) -> void;
    auto insert_with_conflict(const std::string& table, const std::string& columns, const std::string& conflict_column, const std::string& values, const std::string& conflict_value) -> void;

    template<class T>
    auto select(const std::string &table, const std::string &columns, const std::string &condition) -> std::vector<T> {
        if constexpr (!std::is_same_v<T, int> and !std::is_same_v<T, std::string>) {
            report_error("Error selecting from table " + table + ". Unsupported type");
        }

        sqlite3_stmt* stmt;
        auto query = "SELECT " + columns + " FROM " + table + " WHERE " + condition + ";";
        if (sqlite3_prepare( m_dataBase, query.c_str(), -1, &stmt, nullptr ) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            report_error("Error preparing SQL query " + query);
        }

        auto rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return {{}};
        }
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            report_error("Error selecting data from table " + table);
        }

        std::vector<T> result;
        while (rc == SQLITE_ROW) {
            if constexpr (std::is_same_v<T, int>) {
                result.push_back(sqlite3_column_int(stmt, 0));
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                result.push_back(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
            }
            rc = sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        return result;
    }

private:
    auto execute_query(const std::string &query) -> void;
    auto report_error(const std::string& msg) -> void;

    sqlite3* m_dataBase{nullptr};
};
