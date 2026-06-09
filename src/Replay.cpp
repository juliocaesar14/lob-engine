#include "Replay.hpp"
#include "ITCHParser.hpp"
#include "OrderBook.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

struct SymbolState {
    std::ofstream csv;
    uint64_t      msg_count    = 0;
    double        prev_bid_qty = 0;
    double        prev_ask_qty = 0;
    OrderBook     book;
    ReplayStats   stats;
    std::unordered_map<uint64_t, Order> live_orders;
};

ReplayStats Replay::run(const std::string& filepath,
                        const std::string& symbol,
                        bool verbose)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open file: " << filepath << "\n";
        return {};
    }

    const std::vector<std::string> ALL_SYMBOLS = {
        "AAPL", "MSFT", "INTC", "GOOG", "CSCO",
        "FB",   "AMZN", "TSLA", "NVDA", "NFLX"
    };

    std::vector<std::string> tracked;
    if (symbol == "ALL") {
        tracked = ALL_SYMBOLS;
    } else {
        tracked = { symbol };
    }

    std::unordered_map<std::string, SymbolState> states;
    for (const auto& sym : tracked) {
        auto& s = states[sym];
        std::string csv_path = "../analysis/" + sym + "_book_data.csv";
        s.csv.open(csv_path);
        if (!s.csv.is_open()) {
            std::cerr << "ERROR: Cannot open CSV: " << csv_path << "\n";
            return {};
        }
        s.csv << "msg_count,best_bid,best_ask,spread,mid_price,"
              << "bid_qty,ask_qty,ofi\n";
    }

    const uint64_t SNAPSHOT_EVERY = 500;

    uint8_t len_buf[2];
    uint8_t msg_buf[64];
    uint64_t total_messages = 0;

    std::cout << "Starting replay: " << filepath << "\n";
    if (symbol == "ALL")
        std::cout << "Tracking all 10 symbols\n\n";
    else
        std::cout << "Filtering symbol: " << symbol << "\n\n";

    while (file.read(reinterpret_cast<char*>(len_buf), 2)) {
        uint16_t msg_len = (len_buf[0] << 8) | len_buf[1];

        if (msg_len == 0 || msg_len > 64) {
            file.seekg(msg_len, std::ios::cur);
            continue;
        }

        if (!file.read(reinterpret_cast<char*>(msg_buf), msg_len))
            break;

        total_messages++;

        if (total_messages % 1000000 == 0)
            std::cout << "  Processed "
                      << total_messages / 1000000
                      << "M messages...\n";

        ITCHMessageType type = ITCHParser::getType(msg_buf);

        if (type == ITCHMessageType::ADD_ORDER ||
            type == ITCHMessageType::ADD_ORDER_MPID)
        {
            auto msg = ITCHParser::parseAddOrder(msg_buf);
            std::string stock(msg.stock);
            stock.erase(stock.find_last_not_of(' ') + 1);

            auto it = states.find(stock);
            if (it == states.end()) continue;
            SymbolState& s = it->second;

            s.stats.add_orders++;
            s.msg_count++;

            Order order;
            order.order_id = msg.order_ref;
            order.side     = (msg.side == 'B') ? Side::BUY : Side::SELL;
            order.price    = msg.price;
            order.quantity = msg.shares;

            s.live_orders[msg.order_ref] = order;
            auto fills = s.book.addOrder(order);
            s.stats.total_fills += fills.size();
        }
        else if (type == ITCHMessageType::ORDER_CANCEL) {
            auto msg = ITCHParser::parseCancelOrder(msg_buf);

            for (auto& [sym, s] : states) {
                auto oit = s.live_orders.find(msg.order_ref);
                if (oit == s.live_orders.end()) continue;

                s.stats.cancel_orders++;
                s.msg_count++;
                oit->second.quantity -= msg.cancelled_shares;

                if (oit->second.quantity <= 0) {
                    s.book.cancelOrder(msg.order_ref);
                    s.live_orders.erase(oit);
                } else {
                    s.book.modifyOrder(msg.order_ref, oit->second.quantity);
                }
                break;
            }
        }
        else if (type == ITCHMessageType::ORDER_DELETE) {
            auto msg = ITCHParser::parseDeleteOrder(msg_buf);

            for (auto& [sym, s] : states) {
                if (s.live_orders.find(msg.order_ref) == s.live_orders.end())
                    continue;
                s.stats.delete_orders++;
                s.msg_count++;
                s.book.cancelOrder(msg.order_ref);
                s.live_orders.erase(msg.order_ref);
                break;
            }
        }
        else if (type == ITCHMessageType::ORDER_REPLACE) {
            auto msg = ITCHParser::parseReplaceOrder(msg_buf);

            for (auto& [sym, s] : states) {
                auto oit = s.live_orders.find(msg.old_order_ref);
                if (oit == s.live_orders.end()) continue;

                s.stats.replace_orders++;
                s.msg_count++;

                Side side = oit->second.side;
                s.book.cancelOrder(msg.old_order_ref);
                s.live_orders.erase(oit);

                Order new_order;
                new_order.order_id = msg.new_order_ref;
                new_order.side     = side;
                new_order.price    = msg.price;
                new_order.quantity = msg.shares;

                s.live_orders[msg.new_order_ref] = new_order;
                auto fills = s.book.addOrder(new_order);
                s.stats.total_fills += fills.size();
                break;
            }
        }
        else {
            continue;
        }

        for (auto& [sym, s] : states) {
            if (s.msg_count == 0 || s.msg_count % SNAPSHOT_EVERY != 0)
                continue;

            auto bid = s.book.bestBid();
            auto ask = s.book.bestAsk();

            if (bid.has_value() && ask.has_value()) {
                double best_bid = bid.value();
                double best_ask = ask.value();
                double spread   = best_ask - best_bid;
                double mid      = (best_bid + best_ask) / 2.0;
                double bid_qty  = s.book.bestBidQty();
                double ask_qty  = s.book.bestAskQty();
                double ofi      = (bid_qty - s.prev_bid_qty)
                                - (ask_qty - s.prev_ask_qty);

                s.csv << s.msg_count << ","
                      << best_bid << ","
                      << best_ask << ","
                      << spread   << ","
                      << mid      << ","
                      << bid_qty  << ","
                      << ask_qty  << ","
                      << ofi      << "\n";

                s.prev_bid_qty = bid_qty;
                s.prev_ask_qty = ask_qty;
            }
        }
    }

    std::cout << "\n=== REPLAY STATS ===\n";
    std::cout << "Total messages processed: " << total_messages << "\n\n";

    ReplayStats combined;
    combined.total_messages = total_messages;

    for (auto& [sym, s] : states) {
        s.csv.close();
        std::cout << sym << ":\n";
        std::cout << "  Add orders : " << s.stats.add_orders     << "\n";
        std::cout << "  Cancels    : " << s.stats.cancel_orders  << "\n";
        std::cout << "  Deletes    : " << s.stats.delete_orders  << "\n";
        std::cout << "  Replaces   : " << s.stats.replace_orders << "\n";
        std::cout << "  Fills      : " << s.stats.total_fills    << "\n";
        std::cout << "  CSV -> ../analysis/" << sym << "_book_data.csv\n\n";

        combined.add_orders     += s.stats.add_orders;
        combined.cancel_orders  += s.stats.cancel_orders;
        combined.delete_orders  += s.stats.delete_orders;
        combined.replace_orders += s.stats.replace_orders;
        combined.total_fills    += s.stats.total_fills;
    }

    std::cout << "====================\n";
    return combined;
}

