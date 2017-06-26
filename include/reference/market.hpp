#pragma once

#include <cassert>
#include <vector>

#include "instrument.hpp"

namespace toy {
    namespace reference {

        struct level {
            uint64_t bid_qty;
            double bid_prc;
            uint64_t ask_qty;
            double ask_prc;
        };

        class market {
            public:

                using level_list_type = std::vector<level>;

                market(instrument_id iid, uint32_t max_lev) : _iid(iid), _data(max_lev, empty_lev) {}

                auto iid() const { return _iid; }
                auto max_lev() const { return _data.size(); }

                auto last_qty() const { return _last_qty; }
                auto last_prc() const { return _last_prc; }

                auto bid_qty(uint32_t lev) const { return find(lev).bid_qty; }
                auto bid_prc(uint32_t lev) const { return find(lev).bid_prc; }
                auto ask_qty(uint32_t lev) const { return find(lev).ask_qty; }
                auto ask_prc(uint32_t lev) const { return find(lev).ask_prc; }

                auto fill(uint32_t lev, level&& data) {
                    assert(lev < _data.size());
                    _data[lev] = data;
                }

                auto fill(int32_t qty, double prc) {
                    _last_qty = qty;
                    _last_prc = prc;
                }

                auto find(uint32_t lev) const -> level const& {
                    if(lev >= _data.size()) {
                        return empty_lev; 
                    }
                    return _data[lev];
                }

            private:
                instrument_id const _iid;

                static level const empty_lev;

                int32_t _last_qty = 0;
                double _last_prc = 0.0;
                level_list_type _data;
        };

    }
}
