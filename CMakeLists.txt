cmake_minimum_required(VERSION 3.16)
project(huffman_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable memory and undefined behavior checks for debugging
set(SANITIZER_FLAGS "-fsanitize=address,undefined" "-fno-omit-frame-pointer")

add_executable(huffman
    src/main.cpp
    src/huffman.cpp
)

target_include_directories(huffman PRIVATE src)

target_compile_options(huffman PRIVATE
    -Wall -Wextra -Wpedantic
    ${SANITIZER_FLAGS}
)

target_link_options(huffman PRIVATE
    ${SANITIZER_FLAGS}
)

# Test suite excluding main.cpp
add_executable(test_day1
    tests/test_day1.cpp
    src/huffman.cpp
)

target_include_directories(test_day1 PRIVATE src)

target_compile_options(test_day1 PRIVATE
    -Wall -Wextra -Wpedantic
    ${SANITIZER_FLAGS}
)

target_link_options(test_day1 PRIVATE
    ${SANITIZER_FLAGS}
)