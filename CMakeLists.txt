cmake_minimum_required(VERSION 3.23)
project(ngram)

set(TRAIN_EXE_NAME train)
set(PREDICT_EXE_NAME predict)
set(EXECUTABLE_OUTPUT_PATH ../bin)
set(CMAKE_CXX_STANDARD 20)

# Train program
add_executable(${TRAIN_EXE_NAME}
        ./train_dir/train.cpp
        ngram_predictor/ngram_predictor.cpp
        ngram_predictor/ngram_predictor.hpp
        ngram_predictor/ngram_predictor_read.cpp
        ngram_predictor/time_measurements.hpp
        memory_usage.hpp
        memory_usage.cpp ngram_predictor/smoothing.cpp
        ngram_predictor/smoothing.h ngram_predictor/reduce_n_gram.cpp
        ngram_predictor/reduce_n_gram.h
        )

find_package(SQLite3 REQUIRED)
find_package(Boost 1.71.0 COMPONENTS system locale REQUIRED)
find_package(LibArchive REQUIRED)
find_package(Threads REQUIRED)
find_package(TBB REQUIRED)

target_include_directories(${TRAIN_EXE_NAME} PRIVATE .)
target_include_directories(${TRAIN_EXE_NAME} PRIVATE {SQLite3_INCLUDE_DIRS})

target_link_libraries(${TRAIN_EXE_NAME} ${SQLite3_LIBRARIES})
target_link_libraries(${TRAIN_EXE_NAME} Boost::system Boost::locale)
target_link_libraries(${TRAIN_EXE_NAME} archive)
target_link_libraries(${TRAIN_EXE_NAME} Threads::Threads)
target_link_libraries(${TRAIN_EXE_NAME} TBB::tbb)

# Predict program
add_executable(${PREDICT_EXE_NAME}
        ./predict_dir/predict.cpp
        ngram_predictor/ngram_predictor.cpp
        ngram_predictor/ngram_predictor.hpp
        ngram_predictor/ngram_predictor_read.cpp
        ngram_predictor/time_measurements.hpp
        database/database.cpp database/database.hpp
        memory_usage.hpp ngram_predictor/smoothing.cpp
        ngram_predictor/smoothing.h ngram_predictor/reduce_n_gram.cpp
        memory_usage.cpp ngram_predictor/reduce_n_gram.h)

target_include_directories(${PREDICT_EXE_NAME} PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(${PREDICT_EXE_NAME} PRIVATE .)

target_link_libraries(${PREDICT_EXE_NAME} ${SQLite3_LIBRARIES})
target_link_libraries(${PREDICT_EXE_NAME} Boost::system Boost::locale)
target_link_libraries(${PREDICT_EXE_NAME} archive)
target_link_libraries(${PREDICT_EXE_NAME} Threads::Threads)
target_link_libraries(${PREDICT_EXE_NAME} TBB::tbb)


