#include "OrderBook.hpp"
#include <algorithm>
#include <cstdio>

std::vector<Fill> OrderBook::addOrder(Order order) {
    std::vector<Fill> fills = match(order);

    if (order.quantity > 0) {
        order_map_[order.order_id] = order;
        if (order.side == Side::BUY) {
            bids_[order.price].orders.push_back(order);
            bids_[order.price].total_quantity += order.quantity;
        } else {
            asks_[order.price].orders.push_back(order);
            asks_[order.price].total_quantity += order.quantity;
        }
    }
    return fills;
}

std::vector<Fill> OrderBook::match(Order& incoming) {
    std::vector<Fill> fills;

    auto tryMatch = [&](auto& opposite_side) {
        while (incoming.quantity > 0 && !opposite_side.empty()) {
            auto it = opposite_side.begin();
            int64_t level_price = it->first;

            bool crosses = (incoming.side == Side::BUY)
                ? (incoming.price >= level_price)
                : (incoming.price <= level_price);

            if (!crosses) break;

            PriceLevel& level = it->second;

            while (incoming.quantity > 0 && !level.orders.empty()) {
                Order& resting = level.orders.front();
                uint32_t fill_qty = std::min(incoming.quantity, resting.quantity);

                Fill f;
                f.price         = level_price;
                f.quantity      = fill_qty;
                f.buy_order_id  = (incoming.side == Side::BUY)
                    ? incoming.order_id : resting.order_id;
                f.sell_order_id = (incoming.side == Side::SELL)
                    ? incoming.order_id : resting.order_id;

                fills.push_back(f);

                incoming.quantity    -= fill_qty;
                resting.quantity     -= fill_qty;
                level.total_quantity -= fill_qty;

                if (resting.quantity == 0) {
                    order_map_.erase(resting.order_id);
                    level.orders.pop_front();
                }
            }
            if (level.orders.empty())
                opposite_side.erase(it);
        }
    };

    if (incoming.side == Side::BUY) tryMatch(asks_);
    else                             tryMatch(bids_);

    return fills;
}

bool OrderBook::cancelOrder(uint64_t order_id) {
    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) return false;

    Order& o = it->second;

    auto removeFrom = [&](auto& side) {
        auto level_it = side.find(o.price);
        if (level_it == side.end()) return;
        PriceLevel& level = level_it->second;
        auto& q = level.orders;
        for (auto oit = q.begin(); oit != q.end(); ++oit) {
            if (oit->order_id == order_id) {
                level.total_quantity -= oit->quantity;
                q.erase(oit);
                break;
            }
        }
        if (q.empty()) side.erase(level_it);
    };

    if (o.side == Side::BUY) removeFrom(bids_);
    else                      removeFrom(asks_);

    order_map_.erase(it);
    return true;
}

bool OrderBook::modifyOrder(uint64_t order_id, uint32_t new_quantity) {
    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) return false;

    Order& o = it->second;

    auto updateLevel = [&](auto& side) -> bool {
        auto level_it = side.find(o.price);
        if (level_it == side.end()) return false;

        PriceLevel& level = level_it->second;
        for (auto& order : level.orders) {
            if (order.order_id == order_id) {
                level.total_quantity -= order.quantity;
                level.total_quantity += new_quantity;
                order.quantity = new_quantity;
                o.quantity     = new_quantity;
                return true;
            }
        }
        return false;
    };

    if (o.side == Side::BUY) return updateLevel(bids_);
    else                      return updateLevel(asks_);
}

std::optional<int64_t> OrderBook::bestBid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<int64_t> OrderBook::bestAsk() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

uint32_t OrderBook::bestBidQty() const {
    if (bids_.empty()) return 0;
    return bids_.begin()->second.total_quantity;
}

uint32_t OrderBook::bestAskQty() const {
    if (asks_.empty()) return 0;
    return asks_.begin()->second.total_quantity;
}

void OrderBook::print() const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << "ASKS:\n";
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it)
        std::cout << "  " << tickToString(it->first)
                  << " | qty: " << it->second.total_quantity << "\n";
    std::cout << "  ---spread---\n";
    std::cout << "BIDS:\n";
    for (auto& [price, level] : bids_)
        std::cout << "  " << tickToString(price)
                  << " | qty: " << level.total_quantity << "\n";
    std::cout << "==================\n";
}

