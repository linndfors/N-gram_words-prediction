#include "ngram_predictor/ngram_predictor.hpp"
#include "ngram_predictor/time_measurements.hpp"

#include "ngram_predictor/smoothing.h"
#include "ngram_predictor/reduce_n_gram.h"



ngram_predictor::ngram_predictor(int n) 
    : m_n{n}
    , m_words_dict{{"<s>", START_TAG_ID}, {"</s>", END_TAG_ID}, {"<unk>", UNKNOWN_TAG_ID}}
{
    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");
    std::locale::global(loc);
}

void ngram_predictor::check_if_path_is_dir(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Error: input directory " << filename << " does not exist." << std::endl;
        exit(26);
    }
    if (!std::filesystem::is_directory(filename)) {
        std::cerr << "Error: expected directory, got " << filename << std::endl;
        exit(26);
    }
}

auto ngram_predictor::find_word(sqlite3* db, const int n, const ngram_id& context) {
    int rc;
    sqlite3_stmt *stmt;
    if (n == 1) {
        std::string sql = "SELECT ID_WORD_0 FROM n1_grams_frequency WHERE FREQUENCY = (SELECT MAX(FREQUENCY) FROM n1_grams_frequency) ORDER BY FREQUENCY DESC LIMIT 3;";
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return 1;
        }
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int word_id = sqlite3_column_int(stmt, 0);
            std::cout<<"return id: "<<word_id<<std::endl;
            return word_id;
        }
    }
    std::string sql = "SELECT * FROM n" + std::to_string(n) + "_grams_frequency WHERE ";
    for (int i = n-2; i >= 0; i--) {
        if (i < n-2) {
            sql += " AND ";
        }
        sql += "ID_WORD_" + std::to_string(i) + "= ?";
    }
    std::cout<<"sql: "<<sql<<std::endl;
//    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    int reduced_counter = n - 1;
    int context_end = context.size();
    for (int i = n - 2; i >= 0; i--) {
        std::cout<<"id: "<<context[context_end - reduced_counter]<<std::endl;
        sqlite3_bind_int(stmt, i + 1, context[context_end - reduced_counter]);
        reduced_counter --;
    }
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }
    int possible_word_id = 0;
    double prob = -INFINITY;
    std::srand(std::time(nullptr));
    while (sqlite3_step(stmt) == SQLITE_ROW) {
//        std::cout<<"Here"<<std::endl;
        ngram_id current_ngram;
        int last_word_id = 0;
        for (int i = 0; i < n; ++i) {
            int word_id = sqlite3_column_int(stmt, i);
            if (i == n - 1) {
                last_word_id = word_id;
            }
            current_ngram.push_back(word_id);
        }
        double id_prob = smoothing(current_ngram, 0.5);
//        std::cout<<"prob: "<<prob<<std::endl;
        if (id_prob > prob) {
            prob = id_prob;
            possible_word_id = last_word_id;
        }
    }
    sqlite3_finalize(stmt);
    return possible_word_id;
}

auto ngram_predictor::predict_id(const ngram_id& context) const -> id
{
    if (context.empty() || context.size() < m_n - 1){
        std::cerr << "Error: no n-grams have been generated or the context is too short to generate a prediction" << std::endl;
        return -1;
    }
    sqlite3* db;
    char* zErrMsg = nullptr;
    int rc;

    rc = sqlite3_open(DB_PATH, &db);

    if (rc) {
        std::cerr << "Problem with opening database: " << sqlite3_errmsg(db) << std::endl;
        exit(6);
    }


    int res_word_id = 0;
    int current_n = m_n;
    res_word_id = find_word(db, current_n, context);
    std::cout<<"res_word_id: "<<res_word_id<<std::endl;
    while (res_word_id == 0) {
        std::string current_table_name = "n" + std::to_string(current_n) + "_grams_frequency";
//        reduce(current_table_name, current_n);
        current_n--;
//        std::cout<<"current n: "<<current_n<<std::endl;
        res_word_id = find_word(db, current_n, context);
    }

//        if (max_freq_id == 0) {
//            max_freq_id = possible_word_id;
//        }
//        else {
//            double random_double = static_cast<double>(std::rand()) / (RAND_MAX + 1.0);
//            if (random_double > 0.5) {
//                max_freq_id = possible_word_id;
//            }
//        }
//    }

    sqlite3_close(db);
    return res_word_id;
}


auto ngram_predictor::clean_context(ngrams& context) const -> ngrams
{
    int increment = 0;

    for (long i = 0; i < context.size(); ++i) {
        context[i] = boost::locale::fold_case(boost::locale::normalize(context[i]));

        if (context[i].back() == '?' || context[i].back() == '!' || context[i].back() == '.') {
            std::string punctuation = context[i].substr(context[i].size() - 1);
            context[i].erase(context[i].size() - 1);

            // to do - add n-1 <s> and </s> tags
            for (long j = 1; j < m_n; ++j)
                context.insert(context.begin() +  i + j, "</s>");

            for (long j = 1; j < m_n; ++j)
                context.insert(context.begin() + i + m_n - 1 + j, "<s>");

            increment += 2 * m_n - 2;
            // possible to later add punctuation to the end of the word
            // context[i] += punctuation;
        }

        size_t start = 0;
        size_t end = context[i].size() - 1;

        while (start <= end && !isalpha(context[i][start]))
            start++;

        while (end >= start && !isalpha(context[i][end]))
            end--;

        // Remove non-letter characters from the beginning and end
        context[i] = context[i].substr(start, end - start + 1);

        i += increment;
        increment = 0;
    }
    return context;
}


auto ngram_predictor::predict_words(int num_words, ngrams& context) -> ngrams 
{
    auto start = get_current_time_fenced();

    if (context.empty() || context.size() < m_n - 1){
        std::cout<<"1: "<<context.empty()<<std::endl;
        std::cout<<"2: "<<m_n<<std::endl;
        std::cerr << "Error: no n-grams have been generated or the context is too short to generate a prediction" << std::endl;
        return {};
    }

    context = clean_context(context);

    auto context_ids = convert_to_ids(context, false);
    // predict new word based on n previous and add it to context
    for(int i = 0; i < num_words; ++i) {
        auto predicted_id = predict_id(context_ids);
        context_ids.push_back(predicted_id);
    }
    // get back words that was unknown to the model
    auto result = convert_to_words(context_ids);
    for (size_t i = 0; i < context.size(); ++i) {
        result[i] = context[i];
    }

    auto end = get_current_time_fenced();
    m_predicting_time = to_ms(end - start);
    return result;
}


auto ngram_predictor::convert_to_ids(const ngram_predictor::ngrams &ngram, bool train) -> ngram_id 
{
    ngram_id ngram_ids;
    if (train) {
        words_dict_tbb::accessor a;
        for (const auto &word: ngram) {
            if (m_words_dict.find(a, word)) {
                ngram_ids.push_back(a->second);
            } else {
                m_words_dict.insert(a, word);
                {
                    std::lock_guard<std::mutex> lock(m_words_id_mutex);
                    a->second = ++m_last_word_id;
                }
                ngram_ids.push_back(a->second);
            }
        }
        return ngram_ids;
    }
    else {
        sqlite3 *db;
        char *error_message = 0;
        int rc;
        rc = sqlite3_open(DB_PATH, &db);
        if (rc) {
            std::cout << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            exit(6);
        }
        for (const auto &word: ngram) {
            std::string sql = "SELECT ID FROM all_words_id WHERE WORD = '" + word + "';";
            sqlite3_stmt *stmt;
            rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Error preparing statement: find id " << sqlite3_errmsg(db) << std::endl;
                sqlite3_close(db);
                exit(6);
            }
            rc = sqlite3_step(stmt);
            if (rc == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                ngram_ids.push_back(id);
            }
                // if predicting and word does not exist in m_words_dict, add <unk> to the dictionary
            else {
                ngram_ids.push_back(UNKNOWN_TAG_ID);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
        return ngram_ids;
    }
}

auto ngram_predictor::convert_to_id(const ngram_predictor::word &word, bool train) -> id {
    return convert_to_ids({word}, train).front();
}

auto ngram_predictor::convert_to_words(const ngram_predictor::ngram_id &ngram) -> ngrams 
{
    ngrams ngram_words;
    for (const auto& id : ngram) {
        ngram_words.push_back(convert_to_word(id));
    }
    return ngram_words;
}

auto ngram_predictor::convert_to_word(const ngram_predictor::id &id) -> word 
{
    sqlite3 *db;
    char *zErrMsg = nullptr;
    int rc;
    rc = sqlite3_open(DB_PATH, &db);

    if (rc) {
        std::cerr << "Problem with opening database" << std::endl;
        exit(6);
    }
    std::string sql = "SELECT WORD FROM all_words_id WHERE ID = " + std::to_string(id) + ";";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing statement: find word " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char *result = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string word(result);
        return word;
    } else if (rc == SQLITE_DONE) {
        std::cerr << "No rows returned" << std::endl;
    } else {
        std::cerr << "Error executing statement: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return "";
}


void ngram_predictor::print_training_time() const 
{
    std::cout << "Total training time: " << m_total_training_time << " ms" << std::endl;
    std::cout << "  Finding files time: " << m_finding_time << " ms" << std::endl;
    std::cout << "  Reading files time: " << m_reading_time << " ms" << std::endl;
    std::cout << "  Counting ngrams time: " << m_total_training_time - m_writing_ngrams_to_db_time << " ms" << std::endl;
    std::cout << "  Writing ngrams to db time: " << m_writing_ngrams_to_db_time << " ms" << std::endl;
    std::cout << "  Writing words to db time: " << m_writing_words_to_db_time << " ms" << std::endl;
}

void ngram_predictor::print_predicting_time() const 
{
    std::cout << "Predicting time: " << m_predicting_time << " ms" << std::endl;
}

void ngram_predictor::write_ngrams_to_db() 
{
    auto start = get_current_time_fenced();

    sqlite3 *db;
    char *zErrMsg = nullptr;
    int rc;

    rc = sqlite3_open(DB_PATH, &db);

    if (rc) {
        std::cerr<<"Problem with opening database"<<std::endl;
        exit(6);
    }
    std::string table_name = "n" + std::to_string(m_n) + "_grams_frequency";
    std::string sql = "DROP TABLE IF EXISTS " + table_name + ";";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with droping table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sql = "CREATE TABLE " + table_name + "(ID_WORD_0 INT);";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with creating table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }

    if (m_n > 1) {
        for (int j = 1; j < m_n; ++j) {
            std::string col_name = "ID_WORD_" + std::to_string(j);
            std::string sql_add_col = "ALTER TABLE " + table_name + " ADD COLUMN " + col_name + " INT";
            rc = sqlite3_exec(db, sql_add_col.c_str(), callback, nullptr, &zErrMsg);
            if (rc != SQLITE_OK) {
                std::cerr<<"Problem with adding columns"<<std::endl;
                sqlite3_close(db);
                exit(6);
            }
        }
    }
    sql = "ALTER TABLE " + table_name + " ADD COLUMN " + "FREQUENCY" + " INT";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    for (const auto& pair : m_ngram_dict_id) {
        ngram_id words = pair.first;
        auto value = pair.second;
        std::string fields;
        std::string words_id;
        for (int i = 0; i < m_n; ++i) {
            words_id += std::to_string(words[i]);
            words_id += ", ";
            fields += "ID_WORD_" + std::to_string(i);
            fields += ", ";
        }
        sql = "INSERT INTO " + table_name + "(" + fields + "FREQUENCY) VALUES (" + words_id + std::to_string(value) + ");";

        rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
        if (rc != SQLITE_OK) {
            std::cerr<<"Problem with inserting"<<std::endl;
            sqlite3_close(db);
            exit(6);
        }
    }
    rc = sqlite3_exec(db, "COMMIT;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sqlite3_close(db);

    auto end = get_current_time_fenced();
    m_writing_ngrams_to_db_time = to_ms(end - start);
}

void ngram_predictor::write_words_to_db() 
{
    auto start = get_current_time_fenced();

    sqlite3 *db;
    char *zErrMsg = nullptr;
    int rc;

    rc = sqlite3_open(DB_PATH, &db);
    if (rc) {
        std::cerr<<"Problem with open database"<<std::endl;
        exit(6);
    }
    std::string sql = "DROP TABLE IF EXISTS all_words_id;";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr<<"Problem with dropping table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }

    sql = "CREATE TABLE all_words_id (ID INT PRIMARY KEY, WORD VARCHAR(50));";
    rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ) {
        std::cerr<<"Problem with creating table"<<std::endl;
        sqlite3_close(db);
        exit(6);
    }
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    for (const auto& pair : m_words_dict) {
        sqlite3_stmt* stmt;
        sql = "INSERT INTO all_words_id (ID, WORD) VALUES (?, ?)";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc == SQLITE_OK) {
            auto id = pair.second;
            std::string word = pair.first;

            sqlite3_bind_int(stmt, 1, static_cast<int>(id));
            sqlite3_bind_text(stmt, 2, word.c_str(), -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Problem with inserting: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                exit(6);
            }
            sqlite3_finalize(stmt);
        } else {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            exit(6);
        }
    }
    rc = sqlite3_exec(db, "COMMIT;", callback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to commit transaction" << std::endl;
        sqlite3_close(db);
        exit(6);
    }
    sqlite3_close(db);

    auto end = get_current_time_fenced();
    m_writing_words_to_db_time = to_ms(end - start);
}

