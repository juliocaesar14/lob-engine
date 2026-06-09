#include "Replay.hpp"
#include "ITCHParser.hpp"
#include "OrderBook.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

ReplayStats Replay::run(const std::string& filepath,
                        const std::string& symbol,
                        bool verbose)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open file: " << filepath << "\n";
        return {};
    }

    // CSV output for Python analytics
    std::ofstream csv("../analysis/book_data.csv");
    csv << "msg_count,best_bid,best_ask,spread,mid_price,"
        << "bid_qty,ask_qty,ofi\n";

    double prev_bid_qty = 0;
    double prev_ask_qty = 0;
    uint64_t aapl_msg_count = 0;
    const uint64_t SNAPSHOT_EVERY = 500;

    OrderBook book;
    ReplayStats stats;

    std::unordered_map<uint64_t, Order> live_orders;

    uint8_t len_buf[2];
    uint8_t msg_buf[64];

    std::cout << "Starting replay: " << filepath << "\n";
    std::cout << "Filtering symbol: " << symbol << "\n\n";

    while (file.read(reinterpret_cast<char*>(len_buf), 2)) {
        uint16_t msg_len = (len_buf[0] << 8) | len_buf[1];

        if (msg_len == 0 || msg_len > 64) {
            file.seekg(msg_len, std::ios::cur);
            continue;
        }

        if (!file.read(reinterpret_cast<char*>(msg_buf), msg_len))
            break;

        stats.total_messages++;

        if (stats.total_messages % 1000000 == 0)
            std::cout << "  Processed "
                      << stats.total_messages / 1000000
                      << "M messages...\n";

        ITCHMessageType type = ITCHParser::getType(msg_buf);

        if (type == ITCHMessageType::ADD_ORDER ||
            type == ITCHMessageType::ADD_ORDER_MPID)
        {
            auto msg = ITCHParser::parseAddOrder(msg_buf);
            std::string stock(msg.stock);
            stock.erase(stock.find_last_not_of(' ') + 1);
            if (stock != symbol) continue;

            stats.add_orders++;
            aapl_msg_count++;

            Order order;
            order.order_id = msg.order_ref;
            order.side     = (msg.side == 'B') ? Side::BUY : Side::SELL;
            order.price    = msg.price;
            order.quantity = msg.shares;

            live_orders[msg.order_ref] = order;
            auto fills = book.addOrder(order);
            stats.total_fills += fills.size();
        }
        else if (type == ITCHMessageType::ORDER_CANCEL) {
            auto msg = ITCHParser::parseCancelOrder(msg_buf);
            auto it  = live_orders.find(msg.order_ref);
            if (it == live_orders.end()) continue;

            stats.cancel_orders++;
            aapl_msg_count++;
            it->second.quantity -= msg.cancelled_shares;

            if (it->second.quantity <= 0) {
                book.cancelOrder(msg.order_ref);
                live_orders.erase(it);
            } else {
                book.modifyOrder(msg.order_ref, it->second.quantity);
            }
        }
        else if (type == ITCHMessageType::ORDER_DELETE) {
            auto msg = ITCHParser::parseDeleteOrder(msg_buf);
            stats.delete_orders++;
            aapl_msg_count++;
            book.cancelOrder(msg.order_ref);
            live_orders.erase(msg.order_ref);
        }
        else if (type == ITCHMessageType::ORDER_REPLACE) {
            auto msg = ITCHParser::parseReplaceOrder(msg_buf);
            stats.replace_orders++;
            aapl_msg_count++;

            auto it = live_orders.find(msg.old_order_ref);
            if (it == live_orders.end()) continue;

            Side side = it->second.side;
            book.cancelOrder(msg.old_order_ref);
            live_orders.erase(it);

            Order new_order;
            new_order.order_id = msg.new_order_ref;
            new_order.side     = side;
            new_order.price    = msg.price;
            new_order.quantity = msg.shares;

            live_orders[msg.new_order_ref] = new_order;
            auto fills = book.addOrder(new_order);
            stats.total_fills += fills.size();
        }
        else {
            stats.unknown_msgs++;
            continue;
        }

        // Write CSV snapshot every N AAPL messages
        if (aapl_msg_count % SNAPSHOT_EVERY == 0) {
            auto bid = book.bestBid();
            auto ask = book.bestAsk();

            if (bid.has_value() && ask.has_value()) {
                double best_bid = bid.value();
                double best_ask = ask.value();
                double spread   = best_ask - best_bid;
                double mid      = (best_bid + best_ask) / 2.0;

                // Real top-of-book quantities
                double bid_qty = book.bestBidQty();
                double ask_qty = book.bestAskQty();
                double ofi     = (bid_qty - prev_bid_qty)
                                - (ask_qty - prev_ask_qty);

                csv << aapl_msg_count << ","
                    << best_bid << ","
                    << best_ask << ","
                    << spread   << ","
                    << mid      << ","
                    << bid_qty  << ","
                    << ask_qty  << ","
                    << ofi      << "\n";

                prev_bid_qty = bid_qty;
                prev_ask_qty = ask_qty;
            }
        }
    }

    csv.close();
    std::cout << "\nCSV written to analysis/book_data.csv\n";

    book.print();

    std::cout << "\n=== REPLAY STATS ===\n";
    std::cout << "Total messages : " << stats.total_messages  << "\n";
    std::cout << "Add orders     : " << stats.add_orders      << "\n";
    std::cout << "Cancels        : " << stats.cancel_orders   << "\n";
    std::cout << "Deletes        : " << stats.delete_orders   << "\n";
    std::cout << "Replaces       : " << stats.replace_orders  << "\n";
    std::cout << "Total fills    : " << stats.total_fills     << "\n";
    std::cout << "====================\n";

    return stats;
}

