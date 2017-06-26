

#include <iostream>
#include <iomanip>

#include "reference/order.hpp"
#include "reference/market.hpp"

using toy::reference::order_side;

auto operator<<(std::ostream& s, order_side side) -> std::ostream& {
    switch(side) {
    case order_side::buy: return s << "B"; 
    case order_side::sell: return s << 'S';
    default: return s << 'U';
    }
}

auto operator<<(std::ostream& s, toy::reference::order const* po) -> std::ostream& {
    return s << "ODR(" << po->id << ')' << " [" << po->side << ' ' << po->iid << ' '
            << po->qty << "(" << -po->can_qty << ')' << " @ " << po->prc << ']';
}

auto operator<<(std::ostream& s, toy::reference::trade const* pt) -> std::ostream& {
    return s << "TRD(" << pt->id << ')' << " [" << pt->side << ' ' << pt->iid << ' ' << pt->qty << " @ " << pt->prc << ']';
    //return s << "product: " << pt->iid << " " << pt->qty << "@" << pt->prc;
}

auto operator<<(std::ostream& s, toy::reference::market const* pm) -> std::ostream& {
    s << "product: " << pm->iid() << " last " << pm->last_qty() << "@" << pm->last_prc() << "\nPRC: ";
    for(auto i = pm->max_lev(); i > 0; i --) {
        s << std::setw(8) << std::setfill(' ') << pm->bid_prc(i - 1);
    }
    s << " | ";
    for(auto i = 0U; i < pm->max_lev(); i ++) {
        s << std::setw(8) << std::setfill(' ') << pm->ask_prc(i);
    }

    s << "\nQTY: ";
    for(auto i = pm->max_lev(); i > 0; i --) {
        s << std::setw(8) << std::setfill(' ') << pm->bid_qty(i - 1);
    }
    s << " | ";
    for(auto i = 0U; i < pm->max_lev(); i ++) {
        s << std::setw(8) << std::setfill(' ') << pm->ask_qty(i);
    }


    return s;
}
