import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np
import glob
import os
import warnings
warnings.filterwarnings("ignore",message="Creatng..")

csv_files = sorted(glob.glob("*_book_data.csv"))

if not csv_files:
    print("No *_book_data.csv files found. Run the replay first.")
    exit(1)

print(f"Found {len(csv_files)} symbol(s): "
      + ", ".join(f.replace("_book_data.csv", "") for f in csv_files))

summary_rows = []

for csv_path in csv_files:
    symbol = csv_path.replace("_book_data.csv", "")
    print(f"\n--- {symbol} ---")

    df = pd.read_csv(csv_path)
    print(f"  Loaded {len(df)} snapshots")

    df['best_bid']  = df['best_bid']  / 10000
    df['best_ask']  = df['best_ask']  / 10000
    df['spread']    = df['spread']    / 10000
    df['mid_price'] = df['mid_price'] / 10000

    df = df.dropna()
    df = df[df['spread'] > 0]
    df = df[df['spread'] < 1.0]
    df = df[df['best_bid'] > 0]
    df = df[df['best_ask'] > 0]
    df = df.reset_index(drop=True)

    if len(df) < 10:
        print(f"  Not enough data after cleaning, skipping.")
        continue

    print(f"  After cleaning : {len(df)} snapshots")
    print(f"  Price range    : ${df['mid_price'].min():.2f} – ${df['mid_price'].max():.2f}")
    print(f"  Avg spread     : {df['spread'].mean()*100:.2f} cents")

    price_changes = df['mid_price'].diff().fillna(0)
    corr = np.corrcoef(df['ofi'], price_changes)[0, 1]

    fills_path = f"{symbol}_fills.csv"
    has_fills  = os.path.exists(fills_path)
    fills_df   = pd.DataFrame()

    if has_fills:
        fills_df = pd.read_csv(fills_path)
        fills_df['price'] = fills_df['price'] / 10000
        fills_df = fills_df[fills_df['price'] > 0].reset_index(drop=True)
        has_fills = len(fills_df) > 0

    if has_fills:
        vwap = (fills_df['price'] * fills_df['quantity']).sum() / fills_df['quantity'].sum()
        print(f"  Total fills    : {len(fills_df)}")
        print(f"  VWAP           : ${vwap:.2f}")
    else:
        vwap = None
        print(f"  No fills data found.")

    summary_rows.append({
        'Symbol':     symbol,
        'Snapshots':  len(df),
        'Price Low':  f"${df['mid_price'].min():.2f}",
        'Price High': f"${df['mid_price'].max():.2f}",
        'Avg Spread': f"{df['spread'].mean()*100:.2f}¢",
        'OFI Corr':   f"{corr:.4f}",
        'VWAP':       f"${vwap:.2f}" if vwap else "N/A",
    })

    n_rows = 5 if has_fills else 3
    fig = plt.figure(figsize=(14, n_rows * 3.5))
    fig.suptitle(
        f"{symbol} Order Book Analytics — NASDAQ ITCH 5.0 (Dec 30, 2019)",
        fontsize=14, fontweight='bold', y=0.99
    )
    gs = gridspec.GridSpec(n_rows, 2, figure=fig, hspace=0.55, wspace=0.35)

    ax1 = fig.add_subplot(gs[0, :])
    ax1.plot(df.index, df['mid_price'], color='#2196F3', linewidth=0.8, alpha=0.9)
    ax1.fill_between(df.index, df['best_bid'], df['best_ask'],
                     alpha=0.15, color='#2196F3', label='Bid-Ask Range')
    if vwap:
        ax1.axhline(y=vwap, color='orange', linestyle='--',
                    linewidth=1.2, label=f'VWAP: ${vwap:.2f}')
    ax1.set_title('Mid Price Over Time', fontweight='bold')
    ax1.set_ylabel('Price ($)')
    ax1.set_xlabel('Message Snapshot')
    ax1.legend(fontsize=9)
    ax1.grid(True, alpha=0.3)

    ax2 = fig.add_subplot(gs[1, 0])
    ax2.plot(df.index, df['spread'] * 100, color='#F44336', linewidth=0.8)
    ax2.axhline(y=df['spread'].mean()*100, color='black', linestyle='--',
                linewidth=1, label=f"Avg: {df['spread'].mean()*100:.2f}¢")
    ax2.set_title('Bid-Ask Spread Over Time', fontweight='bold')
    ax2.set_ylabel('Spread (cents)')
    ax2.set_xlabel('Message Snapshot')
    ax2.legend(fontsize=9)
    ax2.grid(True, alpha=0.3)

    ax3 = fig.add_subplot(gs[1, 1])
    ax3.hist(df['spread'] * 100, bins=50, color='#FF9800',
             edgecolor='white', linewidth=0.5)
    ax3.axvline(x=df['spread'].mean()*100, color='red', linestyle='--',
                linewidth=1.5, label=f"Mean: {df['spread'].mean()*100:.2f}¢")
    ax3.set_title('Spread Distribution', fontweight='bold')
    ax3.set_xlabel('Spread (cents)')
    ax3.set_ylabel('Frequency')
    ax3.legend(fontsize=9)
    ax3.grid(True, alpha=0.3)

    ax4 = fig.add_subplot(gs[2, 0])
    ax4.plot(df.index, df['ofi'], color='#9C27B0', linewidth=0.6, alpha=0.7)
    ax4.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
    ax4.set_title('Order Flow Imbalance (OFI)', fontweight='bold')
    ax4.set_ylabel('OFI')
    ax4.set_xlabel('Message Snapshot')
    ax4.grid(True, alpha=0.3)

    ax5 = fig.add_subplot(gs[2, 1])
    ax5.scatter(df['ofi'], price_changes, alpha=0.2, s=3, color='#009688')
    ax5.axhline(y=0, color='black', linewidth=0.8)
    ax5.axvline(x=0, color='black', linewidth=0.8)
    ax5.set_title('OFI vs Price Change', fontweight='bold')
    ax5.set_xlabel('OFI')
    ax5.set_ylabel('Mid Price Change ($)')
    ax5.text(0.05, 0.92, f'Corr: {corr:.3f}',
             transform=ax5.transAxes, fontsize=9,
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    ax5.grid(True, alpha=0.3)

    if has_fills:
        ax6 = fig.add_subplot(gs[3, 0])
        max_qty = max(1001, int(fills_df['quantity'].max()) + 1)
        bins    = [0, 50, 100, 200, 500, 1000, max_qty]
        labels  = ['1-50', '51-100', '101-200', '201-500', '501-1000', '1000+']
        fills_df['size_bucket'] = pd.cut(fills_df['quantity'], bins=bins, labels=labels)
        size_counts = fills_df['size_bucket'].value_counts().reindex(labels, fill_value=0)
        ax6.bar(size_counts.index, size_counts.values, color='#3F51B5', edgecolor='white')
        ax6.set_title('Trade Size Distribution', fontweight='bold')
        ax6.set_xlabel('Trade Size (shares)')
        ax6.set_ylabel('Number of Trades')
        ax6.tick_params(axis='x', rotation=30)
        ax6.grid(axis='y', alpha=0.3)

        ax7 = fig.add_subplot(gs[3, 1])
        ax7.plot(fills_df.index, fills_df['price'], color='#795548',
                 linewidth=0.6, alpha=0.8, label='Fill Price')
        ax7.axhline(y=vwap, color='orange', linestyle='--',
                    linewidth=1.5, label=f'VWAP: ${vwap:.2f}')
        ax7.set_title('Fill Prices vs VWAP', fontweight='bold')
        ax7.set_xlabel('Fill Number')
        ax7.set_ylabel('Price ($)')
        ax7.legend(fontsize=9)
        ax7.grid(True, alpha=0.3)

        # U-shaped trade frequency using msg_count as time proxy
        # Divide full msg_count range into 50 equal time buckets
        ax8 = fig.add_subplot(gs[4, :])
        msg_min = fills_df['msg_count'].min()
        msg_max = fills_df['msg_count'].max()
        n_buckets = 50
        bucket_edges = np.linspace(msg_min, msg_max, n_buckets + 1)
        fills_df['time_bucket'] = pd.cut(fills_df['msg_count'],
                                          bins=bucket_edges,
                                          labels=False,
                                          include_lowest=True)
        freq = fills_df.groupby('time_bucket').size().reindex(range(n_buckets), fill_value=0)

        bar_colors = []
        for i in range(n_buckets):
            if i < 5 or i >= n_buckets - 5:
                bar_colors.append('#F44336')  # red for open/close
            else:
                bar_colors.append('#00BCD4')  # teal for midday

        ax8.bar(freq.index, freq.values, color=bar_colors, edgecolor='none', width=1.0)
        ax8.set_title('Trade Frequency Over Time — U-shaped Intraday Pattern\n'
                      '(Red = Market Open/Close, Teal = Midday)',
                      fontweight='bold')
        ax8.set_xlabel('Time Bucket (market open → close)')
        ax8.set_ylabel('Number of Trades')
        ax8.grid(axis='y', alpha=0.3)

        # Annotate open/close/midday
        ax8.annotate('Market Open', xy=(2, freq.iloc[2]),
                     xytext=(6, freq.iloc[2] * 1.05),
                     fontsize=8, color='#F44336',
                     arrowprops=dict(arrowstyle='->', color='#F44336'))
        ax8.annotate('Market Close', xy=(n_buckets - 3, freq.iloc[n_buckets - 3]),
                     xytext=(n_buckets - 12, freq.iloc[n_buckets - 3] * 1.05),
                     fontsize=8, color='#F44336',
                     arrowprops=dict(arrowstyle='->', color='#F44336'))

    out_path = f"{symbol}_analytics.png"
    plt.savefig(out_path, dpi=150, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f"  Chart saved -> {out_path}")

if summary_rows:
    print("\n\n=== CROSS-SYMBOL SUMMARY ===")
    summary_df = pd.DataFrame(summary_rows)
    print(summary_df.to_string(index=False))

    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    fig.suptitle("Cross-Symbol Comparison — NASDAQ ITCH 5.0 (Dec 30, 2019)",
                 fontsize=13, fontweight='bold')

    symbols   = [r['Symbol']                            for r in summary_rows]
    spreads   = [float(r['Avg Spread'].replace('¢','')) for r in summary_rows]
    ofi_corrs = [float(r['OFI Corr'])                   for r in summary_rows]
    vwaps     = [float(r['VWAP'].replace('$',''))
                 if r['VWAP'] != 'N/A' else 0           for r in summary_rows]

    colors = ['#2196F3','#4CAF50','#FF9800','#F44336','#9C27B0',
              '#00BCD4','#FF5722','#607D8B','#795548','#E91E63']

    axes[0].bar(symbols, spreads, color=colors)
    axes[0].set_title('Average Bid-Ask Spread (cents)', fontweight='bold')
    axes[0].set_ylabel('Spread (cents)')
    axes[0].tick_params(axis='x', rotation=45)
    axes[0].grid(axis='y', alpha=0.3)

    bar_colors = ['#F44336' if c < 0 else '#4CAF50' for c in ofi_corrs]
    axes[1].bar(symbols, ofi_corrs, color=bar_colors)
    axes[1].axhline(y=0, color='black', linewidth=0.8)
    axes[1].set_title('OFI–Price Correlation', fontweight='bold')
    axes[1].set_ylabel('Correlation')
    axes[1].tick_params(axis='x', rotation=45)
    axes[1].grid(axis='y', alpha=0.3)

    axes[2].bar(symbols, vwaps, color=colors)
    axes[2].set_title('VWAP by Symbol ($)', fontweight='bold')
    axes[2].set_ylabel('VWAP ($)')
    axes[2].tick_params(axis='x', rotation=45)
    axes[2].grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig("all_symbols_comparison.png", dpi=150,
                bbox_inches='tight', facecolor='white')
    plt.close()
    print("\nComparison chart saved -> all_symbols_comparison.png")

print("\nDone.")


