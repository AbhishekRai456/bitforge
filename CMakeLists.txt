cmake_minimum_required(VERSION 3.16)
project(huffman_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Catch memory and UB issues during development
option(USE_TSAN "Use ThreadSanitizer instead of AddressSanitizer" OFF)
if(USE_TSAN)
    set(SANITIZER_FLAGS "-fsanitize=thread,undefined" "-fno-omit-frame-pointer")
else()
    set(SANITIZER_FLAGS "-fsanitize=address,undefined" "-fno-omit-frame-pointer")
endif()

# Required for std::thread linkage
find_package(Threads REQUIRED)

# Grouped to prevent multiple 'main' definition errors when linking tests
set(CORE_SOURCES
    src/huffman.cpp
    src/bit_io.cpp
    src/encoder.cpp
    src/buffered_io.cpp     
    src/parallel_freq.cpp  
)

add_executable(huffman
    src/main.cpp
    ${CORE_SOURCES}
)
target_include_directories(huffman PRIVATE src)
target_compile_options(huffman PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(huffman PRIVATE ${SANITIZER_FLAGS})
target_link_libraries(huffman PRIVATE Threads::Threads)

add_executable(test_day1
    tests/test_day1.cpp
    src/huffman.cpp
)
target_include_directories(test_day1 PRIVATE src)
target_compile_options(test_day1 PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(test_day1 PRIVATE ${SANITIZER_FLAGS})

add_executable(test_day2
    tests/test_day2.cpp
    ${CORE_SOURCES}
)
target_include_directories(test_day2 PRIVATE src)
target_compile_options(test_day2 PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(test_day2 PRIVATE ${SANITIZER_FLAGS})

add_executable(test_day3
    tests/test_day3.cpp
    ${CORE_SOURCES}
)
target_include_directories(test_day3 PRIVATE src)
target_compile_options(test_day3 PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(test_day3 PRIVATE ${SANITIZER_FLAGS})
target_link_libraries(test_day3 PRIVATE Threads::Threads)