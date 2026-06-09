import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np

# ── Load Data ───────────────────────────────────────────
print("Loading CSV...")
df = pd.read_csv("book_data.csv")
print(f"Loaded {len(df)} snapshots")
print(df.head())

# ── Clean Data ──────────────────────────────────────────
df = df.dropna()
df = df[df['spread'] > 0]
df = df[df['spread'] < 1.0]  # remove outliers
df = df[df['best_bid'] > 0]
df = df[df['best_ask'] > 0]

# Reset index for clean x-axis
df = df.reset_index(drop=True)

print(f"\nAfter cleaning: {len(df)} snapshots")
print(f"Price range: ${df['mid_price'].min():.2f} - ${df['mid_price'].max():.2f}")
print(f"Avg spread:  ${df['spread'].mean():.4f}")
print(f"Avg spread:  {df['spread'].mean()*100:.2f} cents")

# ── Plot ────────────────────────────────────────────────
fig = plt.figure(figsize=(14, 10))
fig.suptitle("AAPL Order Book Analytics — NASDAQ ITCH 5.0 (Dec 30, 2019)",
             fontsize=14, fontweight='bold', y=0.98)

gs = gridspec.GridSpec(3, 2, figure=fig, hspace=0.45, wspace=0.35)

# 1. Mid Price over time
ax1 = fig.add_subplot(gs[0, :])
ax1.plot(df.index, df['mid_price'], color='#2196F3', linewidth=0.8, alpha=0.9)
ax1.set_title('Mid Price Over Time', fontweight='bold')
ax1.set_ylabel('Price ($)')
ax1.set_xlabel('Message Snapshot')
ax1.grid(True, alpha=0.3)
ax1.fill_between(df.index, df['best_bid'], df['best_ask'],
                  alpha=0.15, color='#2196F3', label='Bid-Ask Range')
ax1.legend(fontsize=9)

# 2. Spread over time
ax2 = fig.add_subplot(gs[1, 0])
ax2.plot(df.index, df['spread'] * 100, color='#F44336', linewidth=0.8)
ax2.axhline(y=df['spread'].mean()*100, color='black',
            linestyle='--', linewidth=1, label=f"Avg: {df['spread'].mean()*100:.2f}¢")
ax2.set_title('Bid-Ask Spread Over Time', fontweight='bold')
ax2.set_ylabel('Spread (cents)')
ax2.set_xlabel('Message Snapshot')
ax2.legend(fontsize=9)
ax2.grid(True, alpha=0.3)

# 3. Spread distribution
ax3 = fig.add_subplot(gs[1, 1])
ax3.hist(df['spread'] * 100, bins=50, color='#FF9800', edgecolor='white', linewidth=0.5)
ax3.axvline(x=df['spread'].mean()*100, color='red',
            linestyle='--', linewidth=1.5, label=f"Mean: {df['spread'].mean()*100:.2f}¢")
ax3.set_title('Spread Distribution', fontweight='bold')
ax3.set_xlabel('Spread (cents)')
ax3.set_ylabel('Frequency')
ax3.legend(fontsize=9)
ax3.grid(True, alpha=0.3)

# 4. OFI over time
ax4 = fig.add_subplot(gs[2, 0])
ax4.plot(df.index, df['ofi'], color='#9C27B0', linewidth=0.6, alpha=0.7)
ax4.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
ax4.set_title('Order Flow Imbalance (OFI)', fontweight='bold')
ax4.set_ylabel('OFI')
ax4.set_xlabel('Message Snapshot')
ax4.grid(True, alpha=0.3)

# 5. Price change vs OFI scatter
ax5 = fig.add_subplot(gs[2, 1])
price_changes = df['mid_price'].diff().fillna(0)
ax5.scatter(df['ofi'], price_changes, alpha=0.2, s=3, color='#009688')
ax5.axhline(y=0, color='black', linewidth=0.8)
ax5.axvline(x=0, color='black', linewidth=0.8)
ax5.set_title('OFI vs Price Change', fontweight='bold')
ax5.set_xlabel('OFI')
ax5.set_ylabel('Mid Price Change ($)')
ax5.grid(True, alpha=0.3)

# Correlation
corr = np.corrcoef(df['ofi'], price_changes)[0,1]
ax5.text(0.05, 0.92, f'Corr: {corr:.3f}',
         transform=ax5.transAxes, fontsize=9,
         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

# ── Save ────────────────────────────────────────────────
plt.savefig("aapl_analytics.png", dpi=150, bbox_inches='tight',
            facecolor='white')
print("\nChart saved to analysis/aapl_analytics.png")

# ── Print Summary Stats ──────────────────────────────────
print("\n=== ANALYTICS SUMMARY ===")
print(f"Total snapshots  : {len(df)}")
print(f"Price range      : ${df['mid_price'].min():.2f} - ${df['mid_price'].max():.2f}")
print(f"Avg spread       : {df['spread'].mean()*100:.2f} cents")
print(f"Min spread       : {df['spread'].min()*100:.2f} cents")
print(f"Max spread       : {df['spread'].max()*100:.2f} cents")
print(f"OFI std dev      : {df['ofi'].std():.2f}")
print(f"OFI-price corr   : {corr:.4f}")
print("=========================")

plt.show()

