# Low-Latency Limit Order Book Engine

A limit order book engine written in C++17 that parses real NASDAQ binary market data, matches orders with nanosecond-level performance, and computes market microstructure analytics across 10 of the most traded stocks on NASDAQ.

## What I Built

A limit order book is the core data structure of every electronic exchange in the world. Every time you buy or sell a stock, your order goes into a limit order book which keeps track of all pending buy and sell orders and matches them when prices meet. This project builds one from scratch — not a simulation, but a real engine fed with actual NASDAQ data.

The engine reads raw binary messages directly from a NASDAQ ITCH 5.0 feed, the same format used by professional trading firms. It reconstructs the full order book tick by tick, generates fills every time a buy and sell order cross, tracks every order modification and cancellation, and measures how fast each operation takes down to the nanosecond. On top of that it computes market microstructure signals — bid-ask spread, mid-price movement, and order flow imbalance — from the real tick data using Python.

## Results

I ran the engine on the full NASDAQ feed from December 30, 2019. The file is 8.25GB of raw binary data and contains every single order event across all stocks listed on NASDAQ that day — 268,744,780 messages in total.

For AAPL specifically, the engine handled 698,744 add orders, 114,360,997 deletes, and 21,639,067 order replacements. It reconstructed 73,725 real fills — meaning 73,725 times a buyer and seller agreed on a price and a trade happened.

## How Fast Is It

Speed is everything in trading. A firm that processes orders faster than competitors gets better prices and more profitable executions. This is why latency is measured in nanoseconds, not milliseconds.

I measured latency using rdtsc, which reads the CPU cycle counter directly. This gives nanosecond-level precision compared to standard timing functions which have around 20ns of overhead themselves.

Adding an order to the book takes 554ns on average, with a p50 of 491ns and p99 of 4930ns. The p99 spike comes from Windows OS scheduler interrupts — the operating system occasionally pauses the process to run other tasks. On a dedicated Linux server with CPU pinning, which is what trading firms actually use, this number drops to under 100ns. Cancelling an order takes 171ns on average with a p50 of 150ns. The engine processes 2.69 million orders per second.

The current implementation uses std::map for price levels which gives O(log n) performance. The next optimization is an array-based price ladder which gives O(1) access and much better cache performance since all price levels sit in contiguous memory rather than scattered across the heap.

## Market Microstructure Analytics

After replaying the full ITCH file I took a snapshot of the order book every 500 AAPL messages — recording the best bid, best ask, spread, mid price, and top-of-book quantities. This gave 230,000+ data points which I then analysed in Python.

AAPL traded between $285.80 and $292.50 on that day. The price dipped sharply at market open, recovered through the morning, and closed near the day's high. The average bid-ask spread was 2.09 cents, which is very tight and typical for a highly liquid large-cap stock. The spread was widest at market open when liquidity is thin and tightened quickly as more participants entered the market.

Order flow imbalance measures the difference between buying and selling pressure at the top of the book. When the bid queue grows faster than the ask queue, there is more buying pressure. The correlation between OFI and subsequent price changes was -0.157, suggesting mild mean reversion at this snapshot frequency — when buying pressure builds, price slightly reverses rather than continuing in the same direction.

![AAPL Order Book Analytics](analysis/aapl_analytics.png)

## Multi-Symbol Results

I ran the engine against 10 of the most traded stocks on NASDAQ using the same feed. All 268 million messages were processed for each run, filtering down to the relevant stock. Across all 10 stocks the engine reconstructed 285,432 total trades.

AAPL was the most active stock by far with 698,744 orders and 73,725 fills. MSFT came second with 576,026 orders. NFLX was the quietest with only 138,474 orders and just 62 cancellations, meaning traders rarely changed their mind on Netflix orders that day.

TSLA had an interesting pattern. Despite having fewer orders than MSFT, it generated 41,860 fills compared to MSFT's 37,466. This suggests Tesla traders placed more aggressive orders that crossed the spread immediately rather than sitting and waiting on the book.

AAPL — 698,744 orders, 73,725 fills
MSFT — 576,026 orders, 37,466 fills
INTC — 351,663 orders, 21,143 fills
GOOG — 294,393 orders, 8,108 fills
CSCO — 256,842 orders, 12,920 fills
FB — 242,917 orders, 33,013 fills
AMZN — 232,975 orders, 17,617 fills
TSLA — 228,513 orders, 41,860 fills
NVDA — 202,146 orders, 24,800 fills
NFLX — 138,474 orders, 14,780 fills

## How It Works

The engine has four main components that work together in a pipeline.

The ITCH parser reads the raw binary file two bytes at a time. The first two bytes of every message tell you how long that message is, then it reads exactly that many bytes and looks at the first byte to determine the message type. ITCH uses big-endian byte order which is the opposite of what x86 CPUs use natively, so every multi-byte field needs its bytes reversed using bitwise shifts. Prices are stored as fixed-point integers — the number 2916900 means $291.69 — and divided by 10000 to get the real dollar value.

The order book stores bids in a descending map so the highest buy price is always at the front, and asks in an ascending map so the lowest sell price is always at the front. Each price level holds a FIFO queue of orders — first in, first out — which is how real exchanges prioritise execution. When a new order arrives the engine checks if it crosses the opposite side. A buy crosses if its price is greater than or equal to the best ask. If it crosses, matching begins — consuming orders from the front of each level's queue, generating fills, until the incoming order is fully filled or no more crosses exist.

The replay engine reads the ITCH file message by message and routes each one to the correct handler. Add orders go into the book and are stored in a lookup map by order ID. Delete messages remove the order entirely. Cancel messages reduce its quantity. Replace messages cancel the old order and add a new one with different price or quantity. Every 500 AAPL messages the engine snapshots the current book state into a CSV row.

The benchmark pre-generates 100,000 orders so the generation itself does not affect timing. It runs a warm-up pass to bring relevant data into CPU cache, then measures each individual operation using rdtsc. It detects the CPU frequency automatically by comparing a known number of rdtsc cycles to a wall-clock measurement, then uses that ratio to convert cycles to nanoseconds.

## Project Structure

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
    ├── data/                        ITCH data files (not committed)
    └── CMakeLists.txt

## How to Build

You need g++ 11 or later and CMake 3.14 or later. Clone the repo, create a build folder, run cmake pointing at your MinGW compiler, then run cmake build.

## How to Run

To replay an ITCH file for a specific stock, run lob.exe with the path to your ITCH file and the ticker symbol. For example: lob.exe data/12302019.NASDAQ_ITCH50 AAPL. You can replace AAPL with any NASDAQ ticker.

To run the latency benchmarks, run bench.exe from the build folder.

To generate the analytics charts, go to the analysis folder and run analyze.py with Python. This reads the book_data.csv file generated by the replay and produces a five-chart dashboard.

## Data

NASDAQ ITCH 5.0 sample data is free to download at https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/

