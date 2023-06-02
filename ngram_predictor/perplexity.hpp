#ifndef NGRAM_PERPLEXITY_HPP
#define NGRAM_PERPLEXITY_HPP

#include <sqlite3.h>
#include <iostream>
#include "../database/database.hpp"
#include <cmath>
#include "ngram_predictor.hpp"

double calculate_conditional_prob(ngram_predictor::ngram_id& vals, int n, DataBase& db);

double calculate_ppl(int n, const ngram_predictor::ngram_id& sentence);

#endif //NGRAM_PERPLEXITY_HPP
