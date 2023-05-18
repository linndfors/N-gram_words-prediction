#pragma once
#include "ngram_predictor/ngram_predictor.hpp"

//double smoothing(ngram::ngram_dict_t ngrams_dict, ngram::ngram_t ngram, double d);
double smoothing(ngram_predictor::ngram_id& ngram, double d);