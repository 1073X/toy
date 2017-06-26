#pragma once

#include <iostream>

namespace toy {
    namespace reference {

        class order;
        class trade;
        class market;
    }
}

extern auto operator<<(std::ostream& s, toy::reference::order const*) -> std::ostream&;
extern auto operator<<(std::ostream& s, toy::reference::trade const*) -> std::ostream&;
extern auto operator<<(std::ostream& s, toy::reference::market const*) -> std::ostream&;
