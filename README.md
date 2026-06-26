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

