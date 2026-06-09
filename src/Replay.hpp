#pragma once
#include <string>
#include <cstdint>

struct ReplayStats {
    uint64_t total_messages  = 0;
    uint64_t add_orders      = 0;
    uint64_t cancel_orders   = 0;
    uint64_t delete_orders   = 0;
    uint64_t replace_orders  = 0;
    uint64_t total_fills     = 0;
    uint64_t unknown_msgs    = 0;
};

class Replay {
public:
    // Replay an uncompressed ITCH file into an OrderBook
    // Prints stats every 1M messages
    // Returns summary stats
    static ReplayStats run(const std::string& filepath,
                           const std::string& symbol,
                           bool verbose = false);
};

