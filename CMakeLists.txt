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
    src/cli.cpp
    src/progress.cpp
)

add_executable(huffman
    src/main.cpp
    ${CORE_SOURCES}
)
target_include_directories(huffman PRIVATE src)
target_compile_options(huffman PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(huffman PRIVATE ${SANITIZER_FLAGS})
target_link_libraries(huffman PRIVATE Threads::Threads)

add_executable(test_huffman
    tests/test_huffman.cpp
    src/huffman.cpp
)
target_include_directories(test_huffman PRIVATE src)
target_compile_options(test_huffman PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(test_huffman PRIVATE ${SANITIZER_FLAGS})

add_executable(test_bit_io
    tests/test_bit_io.cpp
    ${CORE_SOURCES}
)
target_include_directories(test_bit_io PRIVATE src)
target_compile_options(test_bit_io PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(test_bit_io PRIVATE ${SANITIZER_FLAGS})

add_executable(test_parallel
    tests/test_parallel.cpp
    ${CORE_SOURCES}
)
target_include_directories(test_parallel PRIVATE src)
target_compile_options(test_parallel PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(test_parallel PRIVATE ${SANITIZER_FLAGS})
target_link_libraries(test_parallel PRIVATE Threads::Threads)

add_executable(test_cli tests/test_cli.cpp ${CORE_SOURCES})
target_include_directories(test_cli PRIVATE src)
target_compile_options(test_cli PRIVATE -Wall -Wextra -Wpedantic ${SANITIZER_FLAGS})
target_link_options(test_cli PRIVATE ${SANITIZER_FLAGS})
target_link_libraries(test_cli PRIVATE Threads::Threads)

add_executable(test_edge_cases tests/test_edge_cases.cpp ${CORE_SOURCES})
target_include_directories(test_edge_cases PRIVATE src)
target_compile_options(test_edge_cases PRIVATE -fsanitize=address,undefined)
target_link_options(test_edge_cases PRIVATE -fsanitize=address,undefined)
target_link_libraries(test_edge_cases PRIVATE Threads::Threads)