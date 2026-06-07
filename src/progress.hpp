#pragma once

#include <cstdint>
#include <cstddef>

// Lightweight stderr progress reporter. Overwrites terminal line via \r.
// Not thread-safe (call only from the main thread).
class ProgressBar {
public:
    ProgressBar(const char* label, uint64_t total_bytes);

    // Throttled to redraw only on 1% changes to avoid terminal flooding.
    void update(uint64_t bytes_done);

    // Erases the progress line and prints a clean "Done" summary.
    void finish();

private:
    const char* label_;
    uint64_t    total_bytes_;
    int         last_pct_;
};