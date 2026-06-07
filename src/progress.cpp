#include "progress.hpp"

#include <cstdio>
#include <cstring>

static constexpr int BAR_WIDTH = 30;

ProgressBar::ProgressBar(const char* label, uint64_t total_bytes)
    : label_(label)
    , total_bytes_(total_bytes)
    , last_pct_(-1)
{
    if (total_bytes_ == 0) return;
    std::fprintf(stderr, "%s: [%*s] %3d%%\r", label_, BAR_WIDTH, "", 0);
    std::fflush(stderr);
}

void ProgressBar::update(uint64_t bytes_done) {
    if (total_bytes_ == 0) return;

    int pct = static_cast<int>(
        (bytes_done * 100ULL) / total_bytes_
    );
    if (pct > 100) pct = 100;

    // Throttle redraws to integer percentages to prevent terminal flicker
    if (pct == last_pct_) return;
    last_pct_ = pct;

    int filled = (pct * BAR_WIDTH) / 100;
    char bar[BAR_WIDTH + 1];
    for (int i = 0; i < BAR_WIDTH; ++i) {
        bar[i] = (i < filled) ? '=' : ' ';
    }
    bar[BAR_WIDTH] = '\0';

    // \r keeps the output on the exact same line
    std::fprintf(stderr, "%s: [%s] %3d%%\r", label_, bar, pct);
    std::fflush(stderr);
}

void ProgressBar::finish() {
    if (total_bytes_ == 0) return;

    // Erase the progress line cleanly before the caller prints the final summary
    std::fprintf(stderr, "\r%80s\r", "");
    std::fflush(stderr);
}