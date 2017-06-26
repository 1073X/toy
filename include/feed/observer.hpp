#pragma once

#include "reference/order.hpp"

namespace toy {
    namespace feed {

        using reference::order;
        using reference::trade;

        class observer {
            public:
                virtual ~observer() {}

                virtual auto add(order const*) -> void = 0;
                virtual auto can(order const*, int64_t) -> void = 0;
                virtual auto amd(order const*, int64_t) -> void = 0;
                virtual auto exe(trade const*) -> void = 0;
        };
    }
}
