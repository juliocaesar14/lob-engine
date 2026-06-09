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
    double   price;
    uint32_t quantity;
};

struct Fill {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double   price;
    uint32_t quantity;
};

struct PriceLevel {
    std::deque<Order> orders;
    uint32_t total_quantity = 0;
};

class OrderBook {
public:
    std::vector<Fill> addOrder(Order order);
    bool cancelOrder(uint64_t order_id);
    bool modifyOrder(uint64_t order_id, uint32_t new_quantity);
    std::optional<double> bestBid() const;
    std::optional<double> bestAsk() const;
    uint32_t bestBidQty() const;
    uint32_t bestAskQty() const;
    void print() const;

private:
    std::map<double, PriceLevel, std::greater<double>> bids_;
    std::map<double, PriceLevel> asks_;
    std::unordered_map<uint64_t, Order> order_map_;

    std::vector<Fill> match(Order& incoming);
};

