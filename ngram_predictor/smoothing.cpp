#include "smoothing.h"

double smoothing(ngram::ngram_dict_t ngrams_dict, ngram::ngram_t ngram, double d) {

    int frequency = ngrams_dict[ngram];
    double first_term_nom = std::max(frequency-d, 0.0);

    double first_term_denom = 0;
    double possible_next_words = 0; //counter for (n-1gram *)
    std::string final_word = ngram[ngram.size()-1]; //last word in given ngram
    double final_word_count = 0;

    for (const auto& [curr_ngram, freq] : ngrams_dict) {

        size_t reduced_ngram_size = curr_ngram.size()-1;
        for (int i = 0; i < reduced_ngram_size; ++i) {
            if (curr_ngram[i] != ngram[i]) break;
            if (i == reduced_ngram_size-1) {
                first_term_denom += freq;
                ++possible_next_words;
            }
        }

        //check if the last word is in current ngram
        if (final_word == curr_ngram[curr_ngram.size() - 1]) {
            ++final_word_count;
        }
    }

    double first_term = first_term_nom/first_term_denom;

    double lambda = d / (first_term_denom * possible_next_words);

    double p_count = final_word_count/ngrams_dict.size();

    double prob = first_term + lambda * p_count;

    // check if in the corpus there is no such ngram, then do smoothing for reduced ngram
    if (prob) return prob;
    else {
        ngram.erase(ngram.begin());
        return smoothing(ngrams_dict, ngram, 0.5);
    }
}
