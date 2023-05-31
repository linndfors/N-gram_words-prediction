#ifndef NGRAM_PERPLEXITY_H
#define NGRAM_PERPLEXITY_H

#include <sqlite3.h>
#include <iostream>
#include "../database/database.hpp"
#include <cmath>
#include "ngram_predictor.hpp"

double calculate_conditional_prob(ngram_predictor::ngram_id& vals, int n, DataBase& db);

double calculate_ppl(int n, ngram_predictor::ngram_id sentence);

#endif //NGRAM_PERPLEXITY_H
