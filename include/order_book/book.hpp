#pragma once

#include <cassert>
#include <map>

#include "reference/order.hpp"
#include "reference/market.hpp"

namespace toy {
    namespace order_book {

        using reference::instrument_id;
        using reference::order_side;
        using reference::level;
        using reference::market;

        class book {
            public:
                book(instrument_id iid) : _iid(iid) {}

                auto verify(int32_t max_lev, int32_t tolerance) {
                    if(_times <= tolerance) {
                        return true;
                    }

                    auto bit = _bids.begin();
                    auto ait = _asks.begin();
                    for(auto i = 0; i < max_lev; i ++) {
                        if(_bids.end() != bit) {
                            if(bit->second <= 0) {
                                log::warn(" ------> Complain! Incomplate bid -", bit->first, bit->second);
                                _times = 0;
                                return false;
                            }
                            bit ++;

                        }
                        if(_asks.end() != ait) {
                            if(ait->second <= 0) {
                                log::warn(" ------> Complain! Incomplate ask -", ait->first, ait->second);
                                _times = 0;
                                return false;
                            }
                        }
                    }
                    return true;
                }

                auto try_extract(market* pmkt) {
                    auto max_lev = pmkt->max_lev();

                    auto bit = find_best(_bids, _bids.begin());
                    auto ait = find_best(_asks, _asks.begin());

                    if(_bids.end() != bit && _asks.end() != ait) {
                        if(bit->first >= ait->first) {
                            log::debug(_iid, " is crossing", bit->first, ">=", ait->first);
                            return false;
                        }
                    }

                    pmkt->fill(_last_qty, _last_prc);
                    for(auto i = 0U; i < max_lev; i ++) {
                        level lev { 0, 0.0, 0, 0.0 };

                        bit = find_best(_bids, bit);
                        if(bit != _bids.end()) {
                            lev.bid_qty = bit->second;
                            lev.bid_prc = bit->first;
                            bit ++;
                        }

                        ait = find_best(_asks, ait);
                        if(ait != _asks.end()) {
                            lev.ask_qty = ait->second;
                            lev.ask_prc = ait->first;
                            ait ++;
                        }

                        pmkt->fill(i, std::move(lev));
                    }

                    _times = 0;
                    return true;
                }


                auto add(order_side side, int64_t qty, double prc) {
                    _times ++;

                    switch(side) {
                    case order_side::buy: add(_bids, qty, prc); break;
                    case order_side::sell: add(_asks, qty, prc); break;
                    default: break;
                    }

                    return _times;
                }

                auto can(order_side side, int64_t qty, double prc) {
                    _times ++;

                    switch(side) {
                    case order_side::buy: can(_bids, qty, prc); break;
                    case order_side::sell: can(_asks, qty, prc); break;
                    default: break;
                    }
                    
                    return _times;
                }

                auto amd(order_side side, int64_t qty, double prc) {
                    _times ++;

                    switch(side) {
                    case order_side::buy: amd(_bids, qty, prc); break;
                    case order_side::sell: amd(_asks, qty, prc); break;
                    default: break;
                    }

                    return _times;
                }

                auto exe(int64_t qty, double prc) {
                    _times ++;

                    _last_qty = qty;
                    _last_prc = prc;
                    
                    return _times;
                }

            private:
                template<typename T>
                auto add(T& qu, int64_t qty, double prc) -> void {
                    auto it = qu.find(prc);
                    if(qu.end() == it) {
                        qu.insert({ prc, qty });
                    }
                    else {
                        it->second += qty;
                    }
                }

                template<typename T>
                auto can(T& qu, int64_t qty, double prc) -> void {
                    auto it = qu.find(prc);
                    if(qu.end() == it) {
                        qu.insert({ prc, -qty });
                    }
                    else {
                        del(qu, it, qty);
                    }
                }

                template<typename T>
                auto amd(T& qu, int64_t qty, double prc) -> void {
                    auto it = qu.find(prc);
                    if(qu.end() == it) {
                        return;
                    }

                    it->second -= qty;
                    if(!it->second) {
                        qu.erase(it);
                    }
                }

                template<typename T>
                auto del(T& qu, typename T::iterator it, int64_t qty) -> void {
                    if(it->second == (int64_t)qty) {
                        qu.erase(it);
                    }
                    else {
                        it->second -= qty;
                    }
                }

                template<typename T>
                auto find_best(T& qu, typename T::iterator it) -> typename T::iterator {
                    while(qu.end() != it) {
                        if(it->second > 0) {
                            break;
                        }
                        it ++;
                    }
                    return it;
                }

            private:
                instrument_id const _iid;

                struct prc_less {
                    constexpr auto operator()(double lh, double rh) const {
                        return lh < rh;
                    }
                };
                std::map<double, int64_t, prc_less> _asks;

                struct prc_greater {
                    constexpr auto operator()(double lh, double rh) const {
                        return lh > rh;
                    }
                };
                std::map<double, int64_t, prc_greater> _bids;

                int32_t _last_qty = 0; 
                double _last_prc = 0.0;

                int32_t _times = 0;
        };

    }
}
