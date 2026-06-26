# Low-Latency Limit Order Book Engine

C++17 limit order book engine that parses real NASDAQ ITCH 5.0 binary data, matches orders at nanosecond latency, and computes microstructure analytics across 10 stocks. Tested on the full December 30 2019 NASDAQ feed, 8.25GB and 268 million messages.

## What it does

Reads raw binary messages from a NASDAQ ITCH 5.0 feed, reconstructs the full order book tick by tick, and matches orders whenever a buy and sell price cross. Tracks every add, cancel, modify, and replace event across the full trading day for up to 10 symbols simultaneously. Every 50 messages per symbol it snapshots the book state to CSV, analysed in Python for bid-ask spread, mid-price, OFI, VWAP, trade size distribution, fill prices, and trade frequency.

The vwap branch extended the fills pipeline to read three parameters from each fills CSV file, msg_count, price, and quantity, and uses all three to compute VWAP, plot fill prices over time, and bucket trade frequency by actual session position rather than row index.

One thing I was unsure about was the OFI correlation being negative across every single symbol. I expected a positive relationship where more buying pressure leads to higher prices, but the data consistently showed the opposite. This could be mean reversion at the 50/500 message snapshot frequency.

## Performance

Latency measured using rdtsc, reading the CPU cycle counter directly for nanosecond precision. The benchmark pre-generates 100,000 orders and runs a warm-up pass before measuring.

Adding an order averages 554ns, p50 491ns, p99 4930ns. Cancelling averages 171ns, p50 150ns. Throughput is 2.69 million orders per second. The p99 spike is Windows OS scheduler interrupts, on a pinned Linux core this drops under 100ns.

Price levels use std::map for O(log n) access. Next step is an array-based price ladder for O(1) and better cache locality.

## Results

Across 10 symbols the engine reconstructed 285,432 total trades. TSLA generated 41,860 fills, the highest fill rate, suggesting aggressive marketable order flow. AAPL averaged a 2.17 cent spread. MSFT showed the strongest OFI-price correlation at -0.39.

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

│   └── *_analytics.png          per-symbol output charts

├── data/                        ITCH data files, not committed

└── CMakeLists.txt

## Quickstart

### 1. Get the data

Download the NASDAQ ITCH 5.0 feed free from https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/

Grab 12302019.NASDAQ_ITCH50.gz, unzip it, and place the raw file inside a data/ folder in the repo root.

### 2. Build

Windows (MinGW)
```powershell
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Mac / Linux
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### 3. Run the replay

Windows
```powershell
.\lob.exe ..\data\12302019.NASDAQ_ITCH50 AAPL
.\lob.exe ..\data\12302019.NASDAQ_ITCH50 ALL
```

Mac / Linux
```bash
./lob ../data/12302019.NASDAQ_ITCH50 AAPL
./lob ../data/12302019.NASDAQ_ITCH50 ALL
```

This writes book_data.csv and fills.csv files to the analysis/ folder. Each fills file contains three columns, msg_count (position in the session), price, and quantity, used downstream to compute VWAP and plot trade activity.

### 4. Run the benchmark

Windows
```powershell
.\bench.exe
```

Mac / Linux
```bash
./bench
```

### 5. Generate charts

```bash
pip install pandas matplotlib numpy
cd analysis
python analyze.py
```

Produces for each symbol a mid price chart with VWAP overlay, bid-ask spread over time and distribution, order flow imbalance and OFI vs price change scatter, trade size distribution, fill prices vs VWAP, and trade frequency over time showing the U-shaped intraday pattern.

Also produces all_symbols_comparison.png comparing spread, OFI correlation, and VWAP across all 10 stocks.


