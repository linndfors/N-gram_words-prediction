cmake_minimum_required(VERSION 3.23)
project(ngram)

set(TRAIN_EXE_NAME train_program)
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
        )

target_include_directories(train_program PRIVATE ngram_predictor)
target_include_directories(train_program PRIVATE src)

find_package(SQLite3 REQUIRED)
if (SQLite3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIRS})
    target_link_libraries(train_program ${SQLite3_LIBRARIES})
endif()

find_package(Boost 1.71.0 COMPONENTS system locale REQUIRED)
target_link_libraries(train_program Boost::system Boost::locale)

find_package(LibArchive REQUIRED)
target_link_libraries(train_program archive)

find_package(Threads REQUIRED)
target_link_libraries(train_program Threads::Threads)

find_package(TBB REQUIRED)
target_link_libraries(train_program TBB::tbb)


# Predict program
add_executable(${PREDICT_EXE_NAME}
        ./predict_dir/predict.cpp
        ngram_predictor/ngram_predictor.cpp
        ngram_predictor/ngram_predictor.hpp
        ngram_predictor/ngram_predictor_read.cpp
        ngram_predictor/time_measurements.hpp
        db_manager/db_manager.cpp db_manager/db_manager.hpp)

target_include_directories(predict PRIVATE ngram_predictor)
target_include_directories(predict PRIVATE src)

target_link_libraries(predict
        ${SQLite3_LIBRARIES}
        Boost::system
        Boost::locale
        archive
        Threads::Threads
        TBB::tbb
        )


target_include_directories(predict PRIVATE ngram_predictor)
target_include_directories(predict PRIVATE src)

find_package(SQLite3 REQUIRED)
if (SQLite3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIRS})
    target_link_libraries(predict ${SQLite3_LIBRARIES})
endif()

find_package(Boost 1.71.0 COMPONENTS system locale REQUIRED)
target_link_libraries(predict Boost::system Boost::locale)

find_package(LibArchive REQUIRED)
target_link_libraries(predict archive)

find_package(Threads REQUIRED)
target_link_libraries(predict Threads::Threads)

find_package(TBB REQUIRED)
target_link_libraries(predict TBB::tbb)


