#include "ngram_predictor.hpp"
#include <boost/locale.hpp>


void ngram_predictor::print_list(std::vector<word> words)  {
    for (const auto& word : words) {
        std::cout << word << " ";
    }
    std::cout<<"\n";
}

auto ngram_predictor::predict_id(const ngram_id& context) -> id {
    if (words_dict.empty() || context.empty() || context.size() < n-1){
        // No n-grams have been generated or the context is too short to generate a prediction
        return -1;
    }

//    std::string next_word;
    id next_id = -1;
    unsigned int max_count = 0;
    for (const auto& [ngram, freq] : ngram_dict_int) {

        if (std::equal(std::prev(context.end(), n-1), context.end(),
                       ngram.begin(), std::prev(ngram.end()))) {
//                for (auto elem:ngram) {
//                    std::cout<<elem<<",";
//                }
//                std::cout<<freq<<std::endl;
            if (freq > max_count) {
                next_id = ngram.back();
                max_count = freq;
            }
        }
        }
    return next_id;
}

auto ngram_predictor::predict_words(int num_words, ngram_str& context) -> ngram_str {
    if (words_dict.empty() || context.empty() || context.size() < n-1){
        // No n-grams have been generated or the context is too short to generate a prediction
        return {};
    }

    for (auto& word: context) {
        word = boost::locale::fold_case(boost::locale::normalize(word));
    }
    // predict new word based on n previous and add it to context
    for(int i = 0; i < num_words; ++i) {
        int predicted_id = predict_id(convert_to_ids(context));
//        print_list(context);
//        std::cout << "predicted id: " << predicted_id << std::endl;
//        std::cout << "predicted word: " << convert_to_word(predicted_id) << std::endl;
        context.push_back(convert_to_word(predicted_id));
    }
    return context;
}

auto ngram_predictor::predict_words(int num_words, std::string& context) -> ngram_str {
    namespace bl = boost::locale;
    auto contents = bl::fold_case(bl::normalize(context));
    bl::boundary::ssegment_index words_index(bl::boundary::word, contents.begin(), contents.end());
    words_index.rule(bl::boundary::word_letters);

    ngram_str vector_of_words;
    for (const auto& word: words_index) {
        vector_of_words.emplace_back(word);
    }

    return predict_words(num_words, vector_of_words);
}

auto ngram_predictor::convert_to_ids(const ngram_predictor::ngram_str &ngram) -> ngram_id {
    ngram_id ngram_ids;
    words_dict_tbb::accessor a;
    for (const auto& word : ngram) {
        words_dict.insert(a, word);
        ngram_ids.push_back(a->second);
    }
    return ngram_ids;
}

auto ngram_predictor::convert_to_id(const ngram_predictor::word &word) -> id {
    words_dict_tbb::accessor a;
    words_dict.insert(a, word);
    if (a->second == 0) {
        a->second = static_cast<int>(words_dict.size());
    }
    return a->second;
}

auto ngram_predictor::convert_to_words(const ngram_predictor::ngram_id &ngram) -> ngram_str {
    ngram_str ngram_words;
    for (const auto& id : ngram) {
        ngram_words.push_back(convert_to_word(id));
    }
    return ngram_words;
}

auto ngram_predictor::convert_to_word(const ngram_predictor::id &id) -> word {
    for (const auto& [word, word_id] : words_dict) {
        if (word_id == id) {
            return word;
        }
    }
    return "";

}