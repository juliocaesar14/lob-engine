#pragma once
#include <cstdint>
#include <map>
#include <unordered_map>
#include <deque>
#include <vector>
#include <optional>
#include <iostream>

enum class Side { BUY, SELL };

struct Order {
    uint64_t order_id;
    Side     side;
    int64_t  price;
    uint32_t quantity;
};

struct Fill {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    int64_t  price; 
    uint32_t quantity;
};

struct PriceLevel {
    std::deque<Order> orders;
    uint32_t total_quantity = 0;
};
inline std::string tickToString(int64_t tick) {
    int64_t dollars = tick / 10000;
    int64_t cents   = (tick % 10000 + 10000) % 10000;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "$%lld.%04lld",
                  (long long)dollars, (long long)cents);
    return buf;
}

class OrderBook {
public:
    [[nodiscard]] std::vector<Fill> addOrder(Order order);
    [[nodiscard]] bool cancelOrder(uint64_t order_id);
    [[nodiscard]] bool modifyOrder(uint64_t order_id, uint32_t new_quantity);

    std::optional<int64_t> bestBid() const;
    std::optional<int64_t> bestAsk() const;
    uint32_t bestBidQty() const;
    uint32_t bestAskQty() const;
    void print() const;

private:
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
    std::map<int64_t, PriceLevel>                         asks_;
    std::unordered_map<uint64_t, Order>                   order_map_;

    std::vector<Fill> match(Order& incoming);
};

