# N-gram_words-prediction

#### To Run 

```
$ ./bin/ngram path n n_words_to_predict context*
```
*context is optional

#### Execution
![image](https://user-images.githubusercontent.com/92580927/228017231-fbad39e6-c43a-44ab-84bb-d71e48ebd5d6.png)

```
$ ./bin/ngram ./corpus.txt 2 6
the cat and dog watched in the

$ ./bin/ngram ./corpus.txt 2 6 the cat
and dog watched in the cat and

$ ./bin/ngram ./corpus.txt 2 6 warmth
enveloped the cat and dog watched in
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
* [Google Web 5gram](https://catalog.ldc.upenn.edu/LDC2006T13): contains English word n-grams and their observed frequency counts (24 GB)

* [Gutenberg Standardized Corpus](https://github.com/pgcorpus/gutenberg): Standardized Project Gutenberg Corpus, 55905 books (3GB counts + 18GB tokens)

* [Harvard Library](http://library.harvard.edu/open-metadata#Harvard-Library-Bibliographic-Dataset): over 12 million bibliographic records for materials held by the Harvard Library, including books, journals, electronic resources, manuscripts, archival materials, scores, audio, video and other materials. (4 GB)

* [Historical Newspapers Yearly N-grams and Entities Dataset](https://data.bris.ac.uk/data/dataset/dobuvuu00mh51q773bo8ybkdz): Yearly time series for the usage of the 1,000,000 most frequent 1-, 2-, and 3-grams from a subset of the British Newspaper Archive corpus, along with yearly time series for the 100,000 most frequent named entities linked to Wikipedia and a list of all articles and newspapers contained in the dataset (3.1 GB)

* [Brown corpus](http://www.sls.hawaii.edu/bley-vroman/brown_corpus.html): the Brown corpus (full name  Brown University Standard Corpus of Present-Day American English) was the first text corpus of American English. The original corpus was published in 1963–1964 by W. Nelson Francis and Henry Kučera (Department of Linguistics, Brown University Providence, Rhode Island, USA). The corpus consists of 1 million words (500 samples of 2000+ words each) of running text of edited English prose printed in the United States during the year 1961 and it was revised and amplified in 1979.

* Other useful datasets for NLP can be found [here](https://github.com/niderhoff/nlp-datasets)

### Additional material
playlist: https://www.youtube.com/watch?v=CXpZnZM63Gg&list=PL8FFE3F391203C98C

### Libraries
* Boost.locale (text separation/cleaning) : https://www.boost.org/doc/libs/1_81_0/libs/locale/doc/html/boundary_analysys.html 
