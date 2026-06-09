#pragma once
#include <cstdint>
#include <string>

// ── ITCH Message Types ──────────────────────────────────
// These are the only message types we care about

enum class ITCHMessageType : char {
    ADD_ORDER           = 'A',
    ADD_ORDER_MPID      = 'F',
    ORDER_CANCEL        = 'X',
    ORDER_DELETE        = 'D',
    ORDER_REPLACE       = 'U',
    TRADE               = 'P',
    UNKNOWN             = '?'
};

// ── Parsed message structs ──────────────────────────────

struct AddOrderMsg {
    uint64_t order_ref;      // unique order ID
    char     side;           // 'B' = buy, 'S' = sell
    uint32_t shares;         // quantity
    char     stock[9];       // ticker symbol (8 chars + null)
    double   price;          // price (decoded from fixed point)
};

struct CancelOrderMsg {
    uint64_t order_ref;
    uint32_t cancelled_shares;
};

struct DeleteOrderMsg {
    uint64_t order_ref;
};

struct ReplaceOrderMsg {
    uint64_t old_order_ref;
    uint64_t new_order_ref;
    uint32_t shares;
    double   price;
};

// ── The Parser Class ────────────────────────────────────

class ITCHParser {
public:
    // Decode one raw ITCH message buffer
    // Returns the message type so caller knows which struct to use
    static ITCHMessageType getType(const uint8_t* buf);

    static AddOrderMsg     parseAddOrder    (const uint8_t* buf);
    static AddOrderMsg     parseAddOrderMPID(const uint8_t* buf);
    static CancelOrderMsg  parseCancelOrder (const uint8_t* buf);
    static DeleteOrderMsg  parseDeleteOrder (const uint8_t* buf);
    static ReplaceOrderMsg parseReplaceOrder(const uint8_t* buf);

private:
    // ITCH is big-endian — these helpers swap bytes
    static uint16_t read16(const uint8_t* buf, int offset);
    static uint32_t read32(const uint8_t* buf, int offset);
    static uint64_t read48(const uint8_t* buf, int offset); // 6-byte int
    static uint64_t read64(const uint8_t* buf, int offset);

    // ITCH prices are fixed-point: divide by 10000
    static double readPrice(const uint8_t* buf, int offset);
};

