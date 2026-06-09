#include "ITCHParser.hpp"
#include <cstring>

// ── Byte readers (big-endian to host) ──────────────────

uint16_t ITCHParser::read16(const uint8_t* buf, int offset) {
    return (uint16_t)(buf[offset] << 8 | buf[offset+1]);
}

uint32_t ITCHParser::read32(const uint8_t* buf, int offset) {
    return ((uint32_t)buf[offset]   << 24) |
           ((uint32_t)buf[offset+1] << 16) |
           ((uint32_t)buf[offset+2] <<  8) |
           ((uint32_t)buf[offset+3]);
}

uint64_t ITCHParser::read48(const uint8_t* buf, int offset) {
    return ((uint64_t)buf[offset]   << 40) |
           ((uint64_t)buf[offset+1] << 32) |
           ((uint64_t)buf[offset+2] << 24) |
           ((uint64_t)buf[offset+3] << 16) |
           ((uint64_t)buf[offset+4] <<  8) |
           ((uint64_t)buf[offset+5]);
}

uint64_t ITCHParser::read64(const uint8_t* buf, int offset) {
    return ((uint64_t)buf[offset]   << 56) |
           ((uint64_t)buf[offset+1] << 48) |
           ((uint64_t)buf[offset+2] << 40) |
           ((uint64_t)buf[offset+3] << 32) |
           ((uint64_t)buf[offset+4] << 24) |
           ((uint64_t)buf[offset+5] << 16) |
           ((uint64_t)buf[offset+6] <<  8) |
           ((uint64_t)buf[offset+7]);
}

double ITCHParser::readPrice(const uint8_t* buf, int offset) {
    return read32(buf, offset) / 10000.0;
}

// ── Message type ────────────────────────────────────────

ITCHMessageType ITCHParser::getType(const uint8_t* buf) {
    switch ((char)buf[0]) {
        case 'A': return ITCHMessageType::ADD_ORDER;
        case 'F': return ITCHMessageType::ADD_ORDER_MPID;
        case 'X': return ITCHMessageType::ORDER_CANCEL;
        case 'D': return ITCHMessageType::ORDER_DELETE;
        case 'U': return ITCHMessageType::ORDER_REPLACE;
        case 'P': return ITCHMessageType::TRADE;
        default:  return ITCHMessageType::UNKNOWN;
    }
}

// ── Add Order (type 'A') ────────────────────────────────
// Offset map per ITCH 5.0 spec:
// 0      = message type (1 byte)
// 1-2    = stock locate (2 bytes)
// 3-4    = tracking number (2 bytes)
// 5-10   = timestamp (6 bytes)
// 11-18  = order ref (8 bytes)
// 19     = side (1 byte)
// 20-23  = shares (4 bytes)
// 24-31  = stock (8 bytes)
// 32-35  = price (4 bytes)

AddOrderMsg ITCHParser::parseAddOrder(const uint8_t* buf) {
    AddOrderMsg msg;
    msg.order_ref = read64(buf, 11);
    msg.side      = (char)buf[19];
    msg.shares    = read32(buf, 20);
    memcpy(msg.stock, buf + 24, 8);
    msg.stock[8]  = '\0';
    msg.price     = readPrice(buf, 32);
    return msg;
}

// ── Add Order with MPID (type 'F') ──────────────────────
// Same as 'A' but has 4 extra bytes for MPID at the end
// Offsets identical up to price

AddOrderMsg ITCHParser::parseAddOrderMPID(const uint8_t* buf) {
    return parseAddOrder(buf); // same offsets for our fields
}

// ── Cancel Order (type 'X') ─────────────────────────────
// 0      = message type
// 1-2    = stock locate
// 3-4    = tracking number
// 5-10   = timestamp
// 11-18  = order ref
// 19-22  = cancelled shares

CancelOrderMsg ITCHParser::parseCancelOrder(const uint8_t* buf) {
    CancelOrderMsg msg;
    msg.order_ref        = read64(buf, 11);
    msg.cancelled_shares = read32(buf, 19);
    return msg;
}

// ── Delete Order (type 'D') ─────────────────────────────
// 0      = message type
// 1-2    = stock locate
// 3-4    = tracking number
// 5-10   = timestamp
// 11-18  = order ref

DeleteOrderMsg ITCHParser::parseDeleteOrder(const uint8_t* buf) {
    DeleteOrderMsg msg;
    msg.order_ref = read64(buf, 11);
    return msg;
}

// ── Replace Order (type 'U') ────────────────────────────
// 0      = message type
// 1-2    = stock locate
// 3-4    = tracking number
// 5-10   = timestamp
// 11-18  = old order ref
// 19-26  = new order ref
// 27-30  = shares
// 31-34  = price

ReplaceOrderMsg ITCHParser::parseReplaceOrder(const uint8_t* buf) {
    ReplaceOrderMsg msg;
    msg.old_order_ref = read64(buf, 11);
    msg.new_order_ref = read64(buf, 19);
    msg.shares        = read32(buf, 27);
    msg.price         = readPrice(buf, 31);
    return msg;
}

