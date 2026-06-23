# Low-Latency Limit Order Book Engine
C++17 limit order book engine that parses real NASDAQ ITCH 5.0 binary data, matches orders, and computes microstructure analytics. Tested on the full December 30 2019 NASDAQ feed, 8.25GB and 268 million messages across all listed stocks.

## What it does
The engine reads raw binary messages from a NASDAQ ITCH 5.0 feed file, reconstructs the full order book tick by tick, and matches orders whenever a buy and sell price cross. It tracks every add, cancel, modify, and replace event across the entire trading day. Every 500 AAPL messages it snapshots the current book state to CSV, which is then analysed in Python to compute bid-ask spread, mid-price movement, and order flow imbalance.

## Performance
Latency is measured using rdtsc, which reads the CPU cycle counter directly and gives nanosecond precision without the 20ns overhead of standard timing functions. The benchmark pre-generates 100,000 orders and runs a warm-up pass to prime the CPU cache before measuring.
Adding an order averages 554ns with a p50 of 491ns and p99 of 4930ns. Cancelling an order averages 171ns with a p50 of 150ns. Throughput is 2.69 million orders per second. The p99 spike is caused by Windows OS scheduler interrupts pausing the process. On a pinned Linux core, which is what trading firms actually use, this drops under 100ns.
The current price level structure uses std::map which gives O(log n) access. The natural next step is an array-based price ladder for O(1) access with better cache locality.

## Results
For AAPL the engine processed 698,744 add orders and reconstructed 73,725 real fills. The average bid-ask spread was 2.09 cents. Order flow imbalance correlated with subsequent price changes at -0.157, indicating mild mean reversion at the 500-message snapshot frequency.
Across 10 stocks on the same feed the engine reconstructed 285,432 total trades. TSLA generated 41,860 fills from only 228,513 orders, the highest fill rate of any stock, suggesting traders placed aggressive marketable orders rather than passive limit orders waiting on the book.

## Project structure
lob-engine/
├── src/
│   ├── OrderBook.hpp/.cpp       core matching engine
│   ├── ITCHParser.hpp/.cpp      NASDAQ ITCH 5.0 binary parser
│   ├── Replay.hpp/.cpp          market data replay engine
│   └── main.cpp                 entry point
├── bench/
│   └── benchmark.cpp            rdtsc latency benchmarks
├── analysis/
│   ├── analyze.py               Python microstructure analytics
│   └── aapl_analytics.png       output charts
├── data/                        ITCH data files, not committed
└── CMakeLists.txt
Build
Requires g++ 11 or later and CMake 3.14 or later.
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
Run
lob.exe data/12302019.NASDAQ_ITCH50 AAPL
Replace AAPL with any NASDAQ ticker. Run bench.exe from the build folder for latency benchmarks. Run analyze.py from the analysis folder to generate charts from book_data.csv.
ITCH data is free to download at https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/



