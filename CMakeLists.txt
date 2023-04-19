cmake_minimum_required(VERSION 3.23)
project(ngram)

set(EXECUTABLE_OUTPUT_PATH ../bin)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp ngram_predictor/ngram_predictor.cpp ngram_predictor/ngram_predictor.hpp ngram_predictor/smoothing.cpp ngram_predictor/smoothing.h)
target_include_directories(${PROJECT_NAME} PRIVATE ngram_predictor)

find_package(Boost 1.71.0 REQUIRED COMPONENTS container)

target_link_libraries(${PROJECT_NAME} Boost::container)
