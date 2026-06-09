#include "OrderBook.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>

// ── rdtsc: reads CPU clock directly in cycles ──────────
inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// ── Get CPU frequency to convert cycles → nanoseconds ──
double getCPUFreqGHz() {
    // Warm up
    auto t1 = std::chrono::high_resolution_clock::now();
    uint64_t c1 = rdtsc();
    volatile uint64_t sum = 0;
    for (int i = 0; i < 10000000; i++) sum += i;
    uint64_t c2 = rdtsc();
    auto t2 = std::chrono::high_resolution_clock::now();

    double ns = std::chrono::duration<double, std::nano>(t2 - t1).count();
    double cycles = (double)(c2 - c1);
    return cycles / ns; // cycles per nanosecond = GHz
}

// ── Statistics helpers ──────────────────────────────────
double mean(std::vector<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

double percentile(std::vector<double>& v, double p) {
    std::sort(v.begin(), v.end());
    int idx = (int)(p / 100.0 * v.size());
    return v[idx];
}

// ── Benchmark addOrder ──────────────────────────────────
void benchAddOrder(double cpuGHz) {
    const int N = 100000;
    std::vector<double> latencies;
    latencies.reserve(N);

    OrderBook book;

    // Pre-generate orders so generation doesn't affect timing
    std::vector<Order> orders;
    orders.reserve(N);
    for (int i = 0; i < N; i++) {
        orders.push_back({
            (uint64_t)i,
            (i % 2 == 0) ? Side::BUY : Side::SELL,
            100.0 + (i % 50) * 0.01,  // spread prices so they don't all cross
            (uint32_t)(100 + i % 100)
        });
    }

    // Warm up the cache
    for (int i = 0; i < 1000; i++)
        book.addOrder(orders[i]);

    // Fresh book for real measurement
    OrderBook bench_book;

    for (int i = 0; i < N; i++) {
        uint64_t start = rdtsc();
        bench_book.addOrder(orders[i]);
        uint64_t end = rdtsc();

        double ns = (end - start) / cpuGHz;
        latencies.push_back(ns);
    }

    std::cout << "\n=== addOrder() Latency ===\n";
    std::cout << "Samples : " << N << "\n";
    std::cout << "Avg     : " << (int)mean(latencies)           << " ns\n";
    std::cout << "p50     : " << (int)percentile(latencies, 50) << " ns\n";
    std::cout << "p90     : " << (int)percentile(latencies, 90) << " ns\n";
    std::cout << "p99     : " << (int)percentile(latencies, 99) << " ns\n";
    std::cout << "p99.9   : " << (int)percentile(latencies, 99.9) << " ns\n";
}

// ── Benchmark cancelOrder ───────────────────────────────
void benchCancelOrder(double cpuGHz) {
    const int N = 100000;
    std::vector<double> latencies;
    latencies.reserve(N);

    // Pre-populate book with orders
    OrderBook book;
    for (int i = 0; i < N; i++) {
        book.addOrder({
            (uint64_t)i,
            Side::BUY,
            100.0 + (i % 100) * 0.01,
            100
        });
    }

    // Measure cancel latency
    for (int i = 0; i < N; i++) {
        uint64_t start = rdtsc();
        book.cancelOrder(i);
        uint64_t end = rdtsc();

        double ns = (end - start) / cpuGHz;
        latencies.push_back(ns);
    }

    std::cout << "\n=== cancelOrder() Latency ===\n";
    std::cout << "Samples : " << N << "\n";
    std::cout << "Avg     : " << (int)mean(latencies)           << " ns\n";
    std::cout << "p50     : " << (int)percentile(latencies, 50) << " ns\n";
    std::cout << "p90     : " << (int)percentile(latencies, 90) << " ns\n";
    std::cout << "p99     : " << (int)percentile(latencies, 99) << " ns\n";
    std::cout << "p99.9   : " << (int)percentile(latencies, 99.9) << " ns\n";
}

// ── Throughput benchmark ────────────────────────────────
void benchThroughput() {
    const int N = 1000000;
    OrderBook book;

    std::vector<Order> orders;
    orders.reserve(N);
    for (int i = 0; i < N; i++) {
        orders.push_back({
            (uint64_t)i,
            (i % 2 == 0) ? Side::BUY : Side::SELL,
            100.0 + (i % 10) * 0.01,
            100
        });
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; i++)
        book.addOrder(orders[i]);
    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();
    double throughput = N / seconds / 1000000.0;

    std::cout << "\n=== Throughput ===\n";
    std::cout << "Orders processed : " << N << "\n";
    std::cout << "Time taken       : " << (int)(seconds * 1000) << " ms\n";
    std::cout << "Throughput       : " << throughput << " M orders/sec\n";
}

// ── Main ────────────────────────────────────────────────
int main() {
    std::cout << "Detecting CPU frequency...\n";
    double cpuGHz = getCPUFreqGHz();
    std::cout << "CPU frequency: " << cpuGHz << " GHz\n";

    benchAddOrder(cpuGHz);
    benchCancelOrder(cpuGHz);
    benchThroughput();

    std::cout << "\n==============================\n";
    std::cout << "  BENCHMARK COMPLETE\n";
    std::cout << "==============================\n";
    return 0;
}

