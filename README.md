# Multithreaded File Compression Engine

A CLI file compressor written in C++17, built from scratch with no external dependencies. Compresses and decompresses files using Huffman Encoding with multithreaded frequency analysis and buffered I/O.

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Usage

```bash
# Compress
./build/huffman --compress --input data.txt --output data.huff

# Decompress
./build/huffman --decompress --input data.huff --output data.txt

# With explicit thread count and no stats
./build/huffman --compress --input data.txt --output data.huff --threads 4 --no-stats

# Help
./build/huffman --help
```

## Benchmark Results

Tested on a 10 MB mixed corpus (5 MB random + 5 MB repetitive text).

|   tool   | output size | ratio |  time  |  sha ok  |
|----------|-------------|-------|--------|----------|
| huffman  |    8.7 MB   | 87.5% | 721ms  |   yes    |
| gzip -6  |    5.0 MB   | 50.1% | 126ms  |   yes    |
| bzip2    |    5.0 MB   | 50.2% | 2713ms |   yes    |

Tested on the Canterbury Corpus (2.6 MB standard benchmark dataset):

|   tool   | output size | ratio |  time  |  sha ok  |
|----------|-------------|-------|--------|----------|
| huffman  |   1.5 MB    | 59.4% | 221ms  |   yes    |
| gzip -6  |   720.6 KB  | 26.2% | 105ms  |   yes    |
| bzip2    |   556.1 KB  | 20.2% | 115ms  |   yes    |

**Why huffman loses on ratio:** gzip and bzip2 run an extra pass before Huffman coding, gzip finds repeated byte sequences and replaces them with back-references (LZ77), bzip2 reorders the data entirely (BWT). A standalone Huffman encoder only looks at individual byte frequencies, so it can't exploit repetition across bytes. Expected tradeoff, not a bug.

Run `./benchmark.sh` for the default synthetic corpus, or pass any file as an argument.

## Architecture

```
src/
├── huffman.cpp       : Frequency table, min-heap tree construction, DFS code generation
├── bit_io.cpp        : MSB-first BitWriter/BitReader, exact bit-count stop condition
├── encoder.cpp       : compress()/decompress() pipeline, binary file format
├── buffered_io.cpp   : fread-based chunked I/O (256KB chunks), RAII FileHandle
├── parallel_freq.cpp : Map-reduce parallel frequency counting via std::thread
├── cli.cpp           : Hand-rolled argv parser
└── progress.cpp      : stderr \r progress bar
```

### Binary File Format

```
[ 8 bytes ] magic number ("HUFF\x00\x01\x00\x00")
[ 8 bytes ] payload bit count (uint64_t, little-endian)
[ N bits  ] serialized huffman tree (DFS: 0=internal node, 1+8bits=leaf), byte-padded
[ M bits  ] compressed payload, byte-padded
```

## Design Decisions

### Why `fread` instead of `std::ifstream`?

`std::ifstream` adds a C++ abstraction layer on top of `fread`, which itself sits on top of `read(2)`. I wanted direct control over chunk size, with explicit 256KB `fread` calls, one syscall covers 256KB instead of going through the streambuf machinery. I also pre-size the read buffer with `fseek`/`ftell` upfront to avoid vector reallocations mid-read.

### Why not `mmap`?

For a sequential single-pass read, `mmap` doesn't actually buy you anything over a well-sized `fread` buffer, the OS handles prefetching the same way either way. It also pulls in virtual memory complexity that isn't relevant to what this tool is doing, so I dropped it.

### Why per-thread local frequency tables instead of a shared atomic array?

A shared `atomic<uint64_t> freq[256]` looks safe but has a cache problem, `freq[0]` and `freq[1]` sit on the same 64-byte cache line, so two threads writing adjacent indices thrash each other through the cache coherence protocol. Giving each thread its own private `freq[256]` on the stack means zero contention during counting. The main thread merges them after `join()` in a simple loop, it's O(256 × threads), basically free.

### Why MSB-first bit packing?

MSB-first means the first bit written ends up in the highest bit position of the byte, which matches how you'd read a binary string left-to-right. It makes debugging with `xxd` straightforward, you can read the hex dump and mentally decode the bits without reversing anything.

### Why store payload bit count instead of padding count?

The decompressor needs to know exactly when to stop so it doesn't consume padding bits as real data. Storing the total bit count gives the reader a clean countdown. Storing padding count instead would mean computing `total_bits - padding` on every exhaustion check, which is a needless subtraction in a tight loop.

### Why hand-roll the CLI parser instead of using `getopt_long`?

Mostly because it's not that much code and I wanted full control over the error messages. `getopt_long` has some quirks with argument ordering and its error output isn't great. The hand-rolled version just walks `argv` linearly, validates everything after parsing, and throws with a clear message on any bad input.

### Why `std::chrono::steady_clock` for timing?

`std::clock()` counts CPU time across all threads, so on a 4-thread run it reports roughly 4× the actual wall time. `system_clock` can jump if the system clock gets adjusted mid-run. `steady_clock` just measures wall time monotonically, it's the right tool for "how long did this actually take."

## Testing

```bash
# build with debug + sanitizers for testing
cmake -B build_debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug

# run the tests
./build_debug/test_huffman      # Core algorithm: 6 tests
./build_debug/test_bit_io       # Bit I/O + serialization: 8 tests
./build_debug/test_parallel     # Buffered I/O + parallel freq: 8 tests
./build_debug/test_cli          # CLI parser: 13 tests
./build_debug/test_edge_cases   # Edge cases: 8 tests
```

Zero errors under `-fsanitize=address,undefined` and Valgrind memcheck.
Zero data races under `-fsanitize=thread,undefined`.

## Known Limitations

1. Decompression progress tracking is not yet implemented. The tool currently displays a "Decompressing: <file>.huff..." message.
2. Compression on random or already compressed data is expected to be poor, as Huffman coding cannot reduce high-entropy input.
