cmake_minimum_required(VERSION 3.14)
project(lob_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")

# Main replay executable
add_executable(lob
    src/main.cpp
    src/OrderBook.cpp
    src/ITCHParser.cpp
    src/Replay.cpp
)
target_include_directories(lob PRIVATE src)

# Benchmark executable
add_executable(bench
    bench/benchmark.cpp
    src/OrderBook.cpp
)
target_include_directories(bench PRIVATE src)

