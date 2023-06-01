#include "ngram_predictor/ngram_predictor.hpp"
#include "ngram_predictor/time_measurements.hpp"

#include "ngram_predictor/perplexity.h"
#include "ngram_predictor/reduce_n_gram.h"

#include "database/database.hpp"
#include <climits>


ngram_predictor::ngram_predictor(int n) 
    : m_n{n}
    , m_words_dict{{"<s>", START_TAG_ID}, {"</s>", END_TAG_ID}, {"<unk>", UNKNOWN_TAG_ID}}
{
    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");
    std::locale::global(loc);

    ngram_id sentence_tag(m_n-1, END_TAG_ID);

    for (auto i = 0; i < m_n - 1; ++i) {
        sentence_tag.emplace_back(START_TAG_ID);
        ngram_dict_id_tbb::accessor a;
        m_ngram_dict_id.insert(a, sentence_tag);
        a->second = INT_MAX;
        sentence_tag.erase(sentence_tag.begin());
    }
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

auto ngram_predictor::find_word(const int n, const ngram_id& context) {
    auto db = DataBase(DB_PATH);
    if (n == 1) {
        auto word_id = db.select<int>("n1_grams_frequency", "ID_WORD_0",
                                  "FREQUENCY = (SELECT MAX(FREQUENCY) FROM n1_grams_frequency) ORDER BY FREQUENCY DESC LIMIT 1;")[0];
        return word_id;
    }

    auto length = context.size();
    std::string condition;
    for (int i = 0; i < n - 1; i++) {
        condition += "ID_WORD_" + std::to_string(i) + " = " + std::to_string(context[length - n + i + 1]);
        if (i != n - 2) {
            condition += " AND ";
        }
    }
    condition += " ORDER BY FREQUENCY DESC LIMIT 3;";

    auto res = db.select<int>("n" + std::to_string(n) + "_grams_frequency", "ID_WORD_" + std::to_string(n-1), condition);

    int max_freq_id = 0;
    std::srand(std::time(nullptr));
    for (auto& word_id : res) {
        if (!max_freq_id) {
            max_freq_id = word_id;
        }
        double random_double = static_cast<double>(std::rand()) / (RAND_MAX + 1.0);
        if (random_double > 0.5) {
            max_freq_id = word_id;
        }
    }
    return max_freq_id;
}

auto ngram_predictor::predict_id(const ngram_id& context) const-> id
{
    if (context.empty() || context.size() < m_n - 1){
        std::cerr << "Error: no n-grams have been generated or the context is too short to generate a prediction" << std::endl;
        return -1;
    }

    int current_n = m_n;
    int res_word_id = find_word(current_n, context);
    while (res_word_id == 0) {
        std::string current_table_name = "n" + std::to_string(current_n) + "_grams_frequency";
//        std::cout<<"current_table_name: "<<current_table_name<<std::endl;
//        reduce(current_table_name, current_n);
        current_n--;
        res_word_id = find_word(current_n, context);
    }

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

            for (long j = 1; j < m_n; ++j)
                context.insert(context.begin() +  i + j, "</s>");

            for (long j = 1; j < m_n; ++j)
                context.insert(context.begin() + i + m_n - 1 + j, "<s>");

            increment += 2 * m_n - 2;
        }

        if (context[i] != "<s>" && context[i] != "</s>") {
            size_t start = 0;
            size_t end = context[i].size() - 1;

            while (start <= end && !isalpha(context[i][start]))
                start++;

            while (end >= start && !isalpha(context[i][end]))
                end--;

            // Remove non-letter characters from the beginning and end
            context[i] = context[i].substr(start, end - start + 1);
        }

        i += increment;
        increment = 0;
    }
    return context;
}

auto ngram_predictor::remove_tags(ngram_predictor::ngrams& words) -> ngrams {
    ngrams res;
    bool capitalize = false;

    for (const auto & word : words) {
        if (word == "</s>") {
            if (!res.empty() && res.back().back() != '.' && res.back() != "</s>") {
                res[res.size() - 1] += ".";
                capitalize = true;
            }
        } else if (word != "<s>") {
            if (capitalize) {
                res.push_back(boost::locale::to_title(word));
                capitalize = false;
            } else {
                res.push_back(word);
            }
        }
    }
    // Capitalize the first word
    if (!res.empty()) {
        res[0] = boost::locale::to_title(res[0]);
    }

    return res;
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
    int predicted_words = 0;

    auto context_ids = convert_to_ids(context, false);

//    double perp = calculate_ppl(m_n, context_ids);
//    std::cout<<"Perplexity index: "<<perp<<std::endl;

    // predict new word based on n previous and add it to context
    while (predicted_words < num_words) {
        auto predicted_id = predict_id(context_ids);
        context_ids.push_back(predicted_id);
        // if predicted word is not a tag
        if (predicted_id > 2) {
            predicted_words++;
        }
    }

    // get back words that were unknown to the model
    auto result = convert_to_words(context_ids);
    for (size_t i = 0; i < context.size(); ++i) {
        result[i] = context[i];
    }

    auto end = get_current_time_fenced();
    m_predicting_time = to_ms(end - start);
    return remove_tags(result);
}

auto ngram_predictor::convert_to_ids(const ngram_predictor::ngrams &ngram, bool train) -> ngram_id 
{
    ngram_id ngram_ids;
    // if training, add new words to m_words_dict
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

    // if predicting, get words from database
    auto db = DataBase(DB_PATH);

    for (const auto &word: ngram) {
        auto id = db.select<int>("all_words_id", "ID", "WORD = '" + word + "';")[0];
        if (!id) {
            // if predicting and word does not exist in m_words_dict, add <unk> to the dictionary
            id = UNKNOWN_TAG_ID;
        }
        ngram_ids.push_back(id);
    }
    return ngram_ids;
}

auto ngram_predictor::convert_to_id(const ngram_predictor::word &word, bool train) -> id {
    return convert_to_ids({word}, train).front();
}

auto ngram_predictor::convert_to_words(const ngram_predictor::ngram_id &ngram) const -> ngrams
{
    ngrams ngram_words;
    for (const auto& id : ngram) {
        ngram_words.push_back(convert_to_word(id));
    }
    return ngram_words;
}

auto ngram_predictor::convert_to_word(const ngram_predictor::id &id) -> word 
{
    auto db = DataBase(DB_PATH);

    auto result = db.select<word>("all_words_id", "WORD", "ID = " + std::to_string(id))[0];
    return result;
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

    // open database
    auto db = DataBase(DB_PATH);
    // drop table if exists
    auto table_name = "n" + std::to_string(m_n) + "_grams_frequency";
    db.drop_table(table_name);

    // create column names using for cycle and create table
    std::string column_names;
    for (int j = 0; j < m_n; ++j) {
        column_names += "ID_WORD_" + std::to_string(j) + " INT, ";
    }
    column_names += "FREQUENCY INT";
    db.create_table(table_name, column_names);

    // insert ngrams into table
    db.begin_transaction();
    for (const auto& pair : m_ngram_dict_id) {
        auto word_ids = pair.first;
        auto freq = static_cast<int>(pair.second);

        // create string for insert values of word ids and frequency
        // and string for column names
        std::string fields;
        std::string insert_values;
        for (int i = 0; i < m_n; ++i) {
            insert_values += std::to_string(word_ids[i]) + ", ";
            fields += "ID_WORD_" + std::to_string(i) + ", ";
        }
        insert_values += std::to_string(freq);
        fields += "FREQUENCY";

        db.insert(table_name, fields, insert_values);
    }
    db.commit_transaction();

    auto end = get_current_time_fenced();
    m_writing_ngrams_to_db_time = to_ms(end - start);
}

void ngram_predictor::write_words_to_db() 
{
    auto start = get_current_time_fenced();
    // open database
    auto db = DataBase(DB_PATH);
    // drop table if exists
    db.drop_table("all_words_id");
    // create table
    db.create_table("all_words_id", "ID INT PRIMARY KEY, WORD VARCHAR(50)");

    // insert words into table
    db.begin_transaction();
    for (const auto& pair : m_words_dict) {
        db.insert("all_words_id", "ID, WORD",
                  std::to_string(static_cast<int>(pair.second)) + ", \"" + pair.first + "\"");
    }
    db.commit_transaction();

    auto end = get_current_time_fenced();
    m_writing_words_to_db_time = to_ms(end - start);
}

