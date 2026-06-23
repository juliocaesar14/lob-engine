
#include "OrderBook.hpp"
#include <cassert>
#include <iostream>

static int passed = 0, failed = 0;

#define CHECK(cond, msg) \
    do { if (cond) { ++passed; } \
         else { std::cerr << "FAIL: " << msg << "\n"; ++failed; } } while(0)

void test_exact_match() {
    OrderBook book;
    book.addOrder({1, Side::SELL, 1000000, 50});
    auto fills = book.addOrder({2, Side::BUY, 1000000, 50});
    CHECK(fills.size() == 1,            "exact match: fill count");
    CHECK(fills[0].quantity == 50,      "exact match: fill qty");
    CHECK(fills[0].price    == 1000000, "exact match: fill price");
    CHECK(!book.bestAsk().has_value(),  "exact match: ask side cleared");
    CHECK(!book.bestBid().has_value(),  "exact match: bid side cleared");
}

void test_partial_fill() {
    OrderBook book;
    book.addOrder({1, Side::SELL, 1000000, 50});
    auto fills = book.addOrder({2, Side::BUY, 1000000, 30});
    CHECK(fills.size() == 1,       "partial fill: one fill event");
    CHECK(fills[0].quantity == 30, "partial fill: fill qty");
    CHECK(book.bestAsk().has_value(),  "partial fill: ask still present");
    CHECK(book.bestAskQty() == 20,     "partial fill: remaining ask qty");
}

void test_no_cross() {
    OrderBook book;
    book.addOrder({1, Side::SELL, 1010000, 100});
    auto fills = book.addOrder({2, Side::BUY, 990000, 100});
    CHECK(fills.empty(),             "no cross: no fills");
    CHECK(book.bestBid() == 990000,  "no cross: bid in book");
    CHECK(book.bestAsk() == 1010000, "no cross: ask in book");
}

void test_cancel() {
    OrderBook book;
    book.addOrder({1, Side::BUY, 1000000, 100});
    bool ok = book.cancelOrder(1);
    CHECK(ok,                          "cancel: returns true");
    CHECK(!book.bestBid().has_value(), "cancel: bid cleared");
}

void test_fifo_priority() {
    OrderBook book;
    book.addOrder({1, Side::SELL, 1000000, 10});
    book.addOrder({2, Side::SELL, 1000000, 10});
    auto fills = book.addOrder({3, Side::BUY, 1000000, 10});
    CHECK(fills.size() == 1,           "fifo: one fill");
    CHECK(fills[0].sell_order_id == 1, "fifo: oldest order matched first");
    CHECK(book.bestAskQty() == 10,     "fifo: second order still on book");
}

int main() {
    test_exact_match();
    test_partial_fill();
    test_no_cross();
    test_cancel();
    test_fifo_priority();
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}

