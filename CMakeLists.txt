cmake_minimum_required(VERSION 3.14)
project(lob_engine)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG -Wall")

add_executable(lob
    src/main.cpp
    src/OrderBook.cpp
    src/ITCHParser.cpp
    src/Replay.cpp
)
target_include_directories(lob PRIVATE src)

add_executable(bench
    bench/benchmark.cpp
    src/OrderBook.cpp
)
target_include_directories(bench PRIVATE src)
target_compile_options(bench PRIVATE -O3 -march=native)

