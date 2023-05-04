#ifndef NGRAM_SMOOTHING_H
#define NGRAM_SMOOTHING_H

#include "ngram_predictor.hpp"

double smoothing(ngram::ngram_dict_t ngrams_dict, ngram::ngram_t ngram, double d);

#endif //NGRAM_SMOOTHING_H
