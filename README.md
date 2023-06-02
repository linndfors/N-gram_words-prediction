# N-gram_words-prediction

## Execution

Firtly you need to compile a program:
```
./compile.sh -d -O
```
Then, you need to train it on the dataset you have:
```
./bin/main predict path_to_dataset ngram_size

```
Finally, to predict some words:
*you should already have n-gram table
```
./bin/main train ngram_size number_of_words_to_predict *context*
```
*context is optional
```
./bin/main predict 2 6 the cat
and dog watched in the cat and
```


#### Speech and Language Processing. Daniel Jurafsky & James H. Martin: https://web.stanford.edu/~jurafsky/slp3/3.pdf

### C++ libraries and packages that support n-gram language models and parallel programming:
 1. KenLM: 
    - https://kheafield.com/code/kenlm/ 
    - https://github.com/kpu/kenlm
 2. SRILM: http://www.speech.sri.com/projects/srilm/
 3. OpenGrm: http://www.opengrm.org/twiki/bin/view/GRM/NGramLibrary
 4. FastText: https://fasttext.cc/ (it is not a direct implementation of N-gram LM, but it represents each word in a text document as a bag of character N-grams, where each N-gram is treated as a separate feature. For example, the word "apple" might be represented as a bag of character trigrams ["app", "ppl", "ple"])

### More about N-gram
https://www.canva.com/design/DAFddjlVQw8/SfpGYVTkFqGQJc-2vsCHjQ/view?utm_content=DAFddjlVQw8&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton

### May be useful

* https://mageswaran1989.medium.com/nlp-what-it-takes-to-model-google-search-suggestions-or-auto-complete-8b94d3671d79

Predicting next word in a search query with python, using [google_wellformed_query](https://huggingface.co/datasets/google_wellformed_query) which has 25K examples

* https://github.com/deskool/nlp-class/blob/master/homework/HW2/materials/code/utils.py

This is file with needed realisation. It is taken from [course](https://www.youtube.com/watch?v=xVf1vcIeqVI) (from which for us is useful [HW2](https://github.com/deskool/nlp-class/tree/master/homework/HW2))

* Some [other realisation](https://www.kaggle.com/code/sauravmaheshkar/auto-completion-using-n-gram-models)

### Datasets
* [Books in plain text for train language models](https://catalog.ldc.upenn.edu/LDC2006T13](https://www.kaggle.com/datasets/paulrohan2020/huge-books-in-plain-text-for-train-language-models)(6 GB)

* [Gutenberg Standardized Corpus](https://github.com/pgcorpus/gutenberg): Standardized Project Gutenberg Corpus, 55905 books (3GB counts + 18GB tokens)


* Other useful datasets for NLP can be found [here](https://github.com/niderhoff/nlp-datasets)

### Additional material
playlist: https://www.youtube.com/watch?v=CXpZnZM63Gg&list=PL8FFE3F391203C98C

### Libraries
* Boost.locale (text separation/cleaning) : https://www.boost.org/doc/libs/1_81_0/libs/locale/doc/html/boundary_analysys.html 
