#pragma once

#include <string>

struct sqlite3;

class DataBase 
{
public:
    explicit DataBase() = default;
    DataBase(const DataBase&) = delete;
    DataBase(DataBase&&) = delete;    
    
    auto operator=(const DataBase&) -> DataBase& = delete;
    auto operator=(DataBase&&) -> DataBase& = delete;

    ~DataBase();

    auto open(const std::string& path) -> bool;
    auto close() -> void;

    auto create_table(const std::string& name, const std::string& columns) -> void;
    auto commit() -> void;
    auto begin_transaction() -> void;

private:
    auto execute_query(const std::string &query) -> void;
    auto report_error() const -> void;

    sqlite3* m_dataBase{nullptr};
};
