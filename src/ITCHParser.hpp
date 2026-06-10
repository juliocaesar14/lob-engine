#pragma once
#include <cstdint>
#include <string>

enum class ITCHMessageType : char {
    ADD_ORDER           = 'A',
    ADD_ORDER_MPID      = 'F',
    ORDER_CANCEL        = 'X',
    ORDER_DELETE        = 'D',
    ORDER_REPLACE       = 'U',
    TRADE               = 'P',
    UNKNOWN             = '?'
};

struct AddOrderMsg {
    uint64_t order_ref;
    char     side;
    uint32_t shares;
    char     stock[9];
    int64_t  price;  
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
    int64_t  price;   
};

class ITCHParser {
public:
    static ITCHMessageType getType(const uint8_t* buf);

    static AddOrderMsg     parseAddOrder    (const uint8_t* buf);
    static AddOrderMsg     parseAddOrderMPID(const uint8_t* buf);
    static CancelOrderMsg  parseCancelOrder (const uint8_t* buf);
    static DeleteOrderMsg  parseDeleteOrder (const uint8_t* buf);
    static ReplaceOrderMsg parseReplaceOrder(const uint8_t* buf);

private:
    static uint16_t read16(const uint8_t* buf, int offset);
    static uint32_t read32(const uint8_t* buf, int offset);
    static uint64_t read48(const uint8_t* buf, int offset);
    static uint64_t read64(const uint8_t* buf, int offset);

    static int64_t readPrice(const uint8_t* buf, int offset);
};


