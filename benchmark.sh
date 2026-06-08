#!/usr/bin/env bash
# Usage: ./benchmark.sh [input_file]
# Compares huffman_engine against gzip and bzip2. Generates synthetic data if no input is provided.

set -euo pipefail

# Config
BINARY="./build/huffman"
TMP_DIR="/tmp/huff_bench"
RESULTS_FILE="$TMP_DIR/results.txt"

# Helpers
die() { echo "error: $1" >&2; exit 1; }

require_tool() {
    command -v "$1" >/dev/null 2>&1 || die "$1 is not installed."
}

bytes_to_human() {
    local b=$1
    if   [ "$b" -ge $((1024*1024)) ]; then printf "%.1f MB" "$(echo "scale=1; $b/1048576" | bc)"
    elif [ "$b" -ge 1024 ];            then printf "%.1f KB" "$(echo "scale=1; $b/1024" | bc)"
    else printf "%d B" "$b"; fi
}

# Preflight
require_tool gzip
require_tool bzip2
require_tool bc
require_tool sha256sum

[ -f "$BINARY" ] || die "huffman binary not found at $BINARY. Run: cmake --build build first."

mkdir -p "$TMP_DIR"
> "$RESULTS_FILE"

# Input corpus
if [ $# -ge 1 ]; then
    INPUT_FILE="$1"
    [ -f "$INPUT_FILE" ] || die "input file not found: $INPUT_FILE"
    echo "using provided input: $INPUT_FILE"
else
    echo -n "generating 10mb synthetic corpus... "
    INPUT_FILE="$TMP_DIR/corpus.bin"

    # 5MB random
    dd if=/dev/urandom of="$TMP_DIR/random.bin" bs=1M count=5 2>/dev/null
    # 5MB repetitive text
    python3 -c "
import sys
chunk = ('the quick brown fox jumps over the lazy dog\n' * 1000).encode()
out = bytearray()
while len(out) < 5*1024*1024:
    out.extend(chunk)
sys.stdout.buffer.write(bytes(out[:5*1024*1024]))
" > "$TMP_DIR/text.bin"

    cat "$TMP_DIR/random.bin" "$TMP_DIR/text.bin" > "$INPUT_FILE"
    echo "ok"
fi

ORIGINAL_SIZE=$(stat -c%s "$INPUT_FILE")
echo ""
echo "input: $INPUT_FILE ($(bytes_to_human $ORIGINAL_SIZE))"
echo ""
printf "%-12s %-12s %-12s %-12s %-10s\n" "tool" "size" "ratio" "time" "sha ok"

# Benchmark function
run_benchmark() {
    local label="$1"
    local compress_cmd="$2"
    local decompress_cmd="$3"
    local compressed="$4"
    local decompressed="$5"

    local start_ns
    start_ns=$(date +%s%N)
    eval "$compress_cmd" 2>/dev/null
    local end_ns
    end_ns=$(date +%s%N)
    local elapsed_ms=$(( (end_ns - start_ns) / 1000000 ))

    local compressed_size
    compressed_size=$(stat -c%s "$compressed")

    local ratio
    ratio=$(echo "scale=1; $compressed_size * 100 / $ORIGINAL_SIZE" | bc)

    # Decompress and verify round-trip integrity
    eval "$decompress_cmd" 2>/dev/null
    local sha_orig sha_decomp
    sha_orig=$(sha256sum "$INPUT_FILE"   | awk '{print $1}')
    sha_decomp=$(sha256sum "$decompressed" | awk '{print $1}')

    local sha_status
    if [ "$sha_orig" = "$sha_decomp" ]; then
        sha_status="yes"
    else
        sha_status="FAIL"
    fi

    printf "%-12s %-12s %-12s %-12s " \
        "$label" \
        "$(bytes_to_human $compressed_size)" \
        "${ratio}%" \
        "${elapsed_ms}ms"
    echo "$sha_status"

    # Append raw data to results file for post-summary
    echo "$label,$compressed_size,$ratio,$elapsed_ms" >> "$RESULTS_FILE"

    # Cleanup
    rm -f "$compressed" "$decompressed"
}

# Run each tool

# huffman_engine
run_benchmark \
    "huffman" \
    "$BINARY --compress   --input '$INPUT_FILE' --output '$TMP_DIR/out.huff' --no-stats" \
    "$BINARY --decompress --input '$TMP_DIR/out.huff' --output '$TMP_DIR/out_decomp.bin' --no-stats" \
    "$TMP_DIR/out.huff" \
    "$TMP_DIR/out_decomp.bin"

# gzip (default level 6)
run_benchmark \
    "gzip -6" \
    "gzip -6 -c '$INPUT_FILE' > '$TMP_DIR/out.gz'" \
    "gzip -d -c '$TMP_DIR/out.gz' > '$TMP_DIR/out_decomp_gz.bin'" \
    "$TMP_DIR/out.gz" \
    "$TMP_DIR/out_decomp_gz.bin"

# bzip2
run_benchmark \
    "bzip2" \
    "bzip2 -c '$INPUT_FILE' > '$TMP_DIR/out.bz2'" \
    "bzip2 -d -c '$TMP_DIR/out.bz2' > '$TMP_DIR/out_decomp_bz2.bin'" \
    "$TMP_DIR/out.bz2" \
    "$TMP_DIR/out_decomp_bz2.bin"

echo ""
echo "done. raw results: $RESULTS_FILE"