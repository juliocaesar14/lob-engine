#include "Replay.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: lob.exe <itch_file> <symbol>\n";
        std::cout << "Example: lob.exe data/12302019.NASDAQ_ITCH50 AAPL\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::string symbol   = argv[2];

    Replay::run(filepath, symbol, false);
    return 0;
}

