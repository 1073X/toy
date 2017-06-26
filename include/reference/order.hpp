#pragma once

#include "instrument.hpp"

namespace toy {
    namespace reference {

#define max_order_id 1000000

        using order_id = uint32_t;
        using trade_id = uint32_t;

        enum struct order_action : char {
            insert = 'N', remove = 'R', amend = 'M', match = 'X', MAX = 'Z'
        };

        enum struct order_side : uint32_t {
            buy = 0, sell = 1, MAX
        };

        struct order {
            using id_type = order_id;
            static id_type const invalid_id = (id_type)-1;

            instrument_id iid;
            order_id id = invalid_id;
            order_side side;
            double prc;
            int64_t qty = 0;
            int64_t book_qty = 0;
            int64_t can_qty = 0;

            order() = default;
            order(order_id id) : id(id) {}
            ~order() {
                id = invalid_id;
                qty = book_qty = can_qty = 0;
            }
        };

        struct trade {
            using id_type = trade_id;
            static id_type const invalid_id = (id_type)-1;

            instrument_id iid;
            order_id oid;
            trade_id id = invalid_id;
            order_side side;
            double prc;
            int64_t qty;

            trade() = default;
            trade(trade_id id) : id(id) {}
            ~trade() {
                id = invalid_id;
            }
        };
    }
}
