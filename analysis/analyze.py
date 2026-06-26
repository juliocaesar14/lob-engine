import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np
import glob
import os

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

    # Fix: divide by 10000 to convert fixed-point integers to dollars
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

    print(f"  After cleaning: {len(df)} snapshots")
    print(f"  Price range: ${df['mid_price'].min():.2f} - ${df['mid_price'].max():.2f}")
    print(f"  Avg spread:  {df['spread'].mean()*100:.2f} cents")

    price_changes = df['mid_price'].diff().fillna(0)
    corr = np.corrcoef(df['ofi'], price_changes)[0, 1]

    summary_rows.append({
        'Symbol':     symbol,
        'Snapshots':  len(df),
        'Price Low':  f"${df['mid_price'].min():.2f}",
        'Price High': f"${df['mid_price'].max():.2f}",
        'Avg Spread': f"{df['spread'].mean()*100:.2f}¢",
        'OFI Corr':   f"{corr:.4f}",
    })

    fig = plt.figure(figsize=(14, 10))
    fig.suptitle(
        f"{symbol} Order Book Analytics — NASDAQ ITCH 5.0 (Dec 30, 2019)",
        fontsize=14, fontweight='bold', y=0.98
    )
    gs = gridspec.GridSpec(3, 2, figure=fig, hspace=0.45, wspace=0.35)

    ax1 = fig.add_subplot(gs[0, :])
    ax1.plot(df.index, df['mid_price'], color='#2196F3', linewidth=0.8, alpha=0.9)
    ax1.fill_between(df.index, df['best_bid'], df['best_ask'],
                     alpha=0.15, color='#2196F3', label='Bid-Ask Range')
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

    out_path = f"{symbol}_analytics.png"
    plt.savefig(out_path, dpi=150, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f"  Chart saved -> {out_path}")

if summary_rows:
    print("\n\n=== CROSS-SYMBOL SUMMARY ===")
    summary_df = pd.DataFrame(summary_rows)
    print(summary_df.to_string(index=False))

    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle("Cross-Symbol Comparison — NASDAQ ITCH 5.0 (Dec 30, 2019)",
                 fontsize=13, fontweight='bold')

    symbols   = [r['Symbol']                           for r in summary_rows]
    spreads   = [float(r['Avg Spread'].replace('¢','')) for r in summary_rows]
    ofi_corrs = [float(r['OFI Corr'])                  for r in summary_rows]

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

    plt.tight_layout()
    plt.savefig("all_symbols_comparison.png", dpi=150,
                bbox_inches='tight', facecolor='white')
    plt.close()
    print("\nComparison chart saved -> all_symbols_comparison.png")

print("\nDone.")

