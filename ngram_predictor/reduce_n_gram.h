#ifndef NGRAM_REDUCE_N_GRAM_H
#define NGRAM_REDUCE_N_GRAM_H

#include <iostream>
#include <vector>
#include "ngram_predictor.hpp"
#include <sqlite3.h>

void reduce(std::string const& table_name, int n);

#endif //NGRAM_REDUCE_N_GRAM_H
