# Low-Latency Limit Order Book Engine
C++17 limit order book engine that parses real NASDAQ ITCH 5.0 binary data, matches orders, and computes microstructure analytics. Tested on the full December 30 2019 NASDAQ feed, 8.25GB and 268 million messages.
Performance
Latency measured with rdtsc, the CPU cycle counter, which gives nanosecond precision.
Add order averages 554ns, with a p50 of 491ns and p99 of 4930ns. Cancel averages 171ns with a p50 of 150ns. Throughput is 2.69 million orders per second. The p99 spike is Windows scheduler jitter and drops under 100ns on a pinned Linux core.
Results
For AAPL the engine processed 698,744 orders and reconstructed 73,725 real fills. The average bid-ask spread was 2.09 cents. Order flow imbalance correlated with subsequent price changes at -0.157, indicating mild mean reversion at the 500-message snapshot frequency.
Across 10 stocks on the same feed the engine reconstructed 285,432 total trades. AAPL was the most active with 73,725 fills. TSLA generated 41,860 fills from only 228,513 orders, more fills per order than any other stock, suggesting aggressive marketable orders rather than passive limit orders sitting on the book.
Build
Requires g++ 11 or later and CMake 3.14 or later.
mkdir build, cd build, cmake with MinGW Makefiles and Release build type, then cmake build.
Run
Run lob.exe with the path to your ITCH file and a ticker, for example lob.exe data/12302019.NASDAQ ITCH50 AAPL. Run bench.exe for latency benchmarks. Run analyze.py from the analysis folder to generate charts from book data.csv.
ITCH data is free to download at https://emi.nasdaq.com/ITCH/Nasdaq ITCH/


