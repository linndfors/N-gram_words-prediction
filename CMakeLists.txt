cmake_minimum_required(VERSION 3.23)
project(ngram)

set(EXECUTABLE_OUTPUT_PATH ../bin)

set(CMAKE_CXX_STANDARD 20)


add_executable(${PROJECT_NAME} main.cpp ngram_predictor/ngram_predictor.cpp ngram_predictor/ngram_predictor.hpp
        ngram_predictor/ngram_predictor_read.cpp ngram_predictor/time_measurements.hpp ngram_predictor/smoothing.cpp ngram_predictor/smoothing.h)

target_include_directories(${PROJECT_NAME} PRIVATE ngram_predictor)
target_include_directories(${PROJECT_NAME} PRIVATE src)
find_package(SQLite3 REQUIRED)
if (SQLite3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SQLite3_LIBRARIES})
endif()

find_package(Boost 1.71.0 COMPONENTS system locale REQUIRED)
target_link_libraries(${PROJECT_NAME} Boost::system Boost::locale)

find_package(LibArchive REQUIRED)
target_link_libraries(${PROJECT_NAME} archive)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

find_package(TBB REQUIRED)
target_link_libraries(${PROJECT_NAME} TBB::tbb)
