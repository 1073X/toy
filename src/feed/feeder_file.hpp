#pragma once

#include <cassert>
#include <cmath>
#include <thread>
#include <chrono>
#include <fstream>
#include <locale>

#include "reference/container.hpp"
#include "feed/feeder.hpp"

extern auto SIGTERM_handler(int) -> void;

namespace toy {
    namespace feed {

#define LOG_WARN(FIELD, LINE) log::warn("PARSING_WARN - ", LINE, "\t- ["#FIELD"]")
#define LOG_ERR(FIELD, LINE) log::error("PARSING_ERR  - ", LINE, "\t- ["#FIELD"]")

        using reference::order_action;
        using reference::order_side;
        using reference::order;
        using reference::trade;

        auto trim(const char*& str) {
            while(std::isspace(*str)) {
                str ++;
            }
        }

        auto extract_act(const char*& str) {
            trim(str);
            auto act = (order_action)*str;
            str ++;
            trim(str);

            if(',' != *str) {
                return order_action::MAX;
            }

            str ++;
            return act;
        }

        auto extract_uint(const char*& str, char delim = ',') {
            trim(str);
            auto val = 0U;
            while(std::isdigit(*str)) {
                val = val * 10 + (*str - '0');
                str ++;
            }
            trim(str);

            if(*str != delim) {
                return (int64_t)-1L;
            }

            str ++; // skip delim
            return (int64_t)val;
        }

        auto extract_side(const char*& str) {
            trim(str);
            auto side = order_side::MAX;
            auto ch = *(str++);
            if('B' == ch) {
                side = order_side::buy;
            }
            else if('S' == ch) {
                side = order_side::sell;
            }
            trim(str);

            if(*str != ',') {
                return order_side::MAX;
            }

            str ++;
            return side;
        }

        auto extract_prc(const char*& str) {
            trim(str);
            auto val = 0.0;
            while(std::isdigit(*str)) {
                val = val * 10 + (*str - '0');
                str ++;
            }
            if(*str == '.') {
                str ++;

                auto exponent = 0.1;
                while(std::isdigit(*str)) {
                    val += (*str - '0') * exponent;
                    exponent /= 10;
                    str ++;
                }

                // ?????? 100. ????? well, let it be
            }
            trim(str);

            if('\0' != *str) {
                return -1.0;
            }

            return val;
        }

        class feeder_file : public feeder {
            using order_container = reference::container<order>;
            using trade_container = reference::container<trade>;

            public:
                feeder_file(std::string const& pathname, bool tolarant, bool log_comment)
                    : _pathname(pathname), _tolerant(tolarant), _log_comment(log_comment) {}

            private: // feed
                auto start() -> bool override {
                    stop(); // anyway ...

                    _stop = false;
                    _thrd = std::thread([&]() {
                        std::ifstream s(_pathname);
                        if(!s.good()) {
                            log::error("failed to open market data for replay", _pathname);
                            SIGTERM_handler(SIGTERM);
                            return;
                        }

                        log::info("feeder_file starting ... ", _tolerant ? "tolerant" : "strict");

                        auto line_num = 0;

                        while(!_stop) {
                            line_num ++;

                            std::string line; std::getline(s, line);
                            if(line.empty()) {
                                _stop = s.eof();
                                continue;
                            }

                            if('#' == line[0]) {
                                handle_comment(line.c_str(), line_num);
                                continue;
                            }
                      
                            auto str = line.c_str();
                            switch(extract_act(str)) {
                                case order_action::insert: handle_add(str, line_num); break; 
                                case order_action::remove: handle_can(str, line_num); break; 
                                case order_action::amend: handle_amd(str, line_num); break; 
                                case order_action::match: handle_exe(str, line_num); break;
                                default: LOG_ERR(illegal_act, line_num); continue; 
                            }
                        }

                        s.close();

                        log::warn("feeder_file stopped");

                        SIGTERM_handler(SIGTERM);
                    });

                    return true;
                }

                auto stop() -> void {
                    if(!_thrd.joinable()) {
                        return;
                    }

                    _stop = true;
                    _thrd.join();
                }

            private:
                auto handle_add(const char* str, uint32_t line_num) -> void {
                    auto iid = extract_uint(str);
                    if(iid <= 0) {
                        LOG_ERR(illegal_iid, line_num);
                        return;
                    }

                    auto id = extract_uint(str);
                    if(id <= 0) {
                        LOG_ERR(illegal_id, line_num);
                        return;
                    }

                    auto side = extract_side(str);
                    if(order_side::MAX == side) {
                        LOG_ERR(illegal_side, line_num);
                        return;
                    }

                    auto qty = extract_uint(str);
                    if(qty <= 0) {
                        LOG_ERR(illegal_qty, line_num);
                        return;
                    }

                    auto prc = extract_prc(str);
                    if(prc <= 0.0) {
                        LOG_ERR(illegal_prc, line_num);
                        _orders.remove(id);
                        return;
                    }

                    auto po = _orders.retrieve(id);
                    assert(po != nullptr);
                    if(po->qty > 0) {
                        LOG_ERR(duplicated, line_num);
                        return;
                    }
                    else if(po->qty < 0) {
                        LOG_ERR(corrupted, line_num);
                        return;
                    }
                    
                    if(po->can_qty > qty) {
                        LOG_ERR(over_can, line_num);
                        po->qty = -1;
                        return;
                    }

                    if(po->book_qty > qty) {
                        LOG_ERR(over_amd, line_num);
                        po->qty = -1;
                        return;
                    }

                    po->iid = iid;
                    po->qty = qty;// this is the only place that order qty is assigned!

                    if(!po->book_qty && !po->can_qty) {
                        po->side = side;
                        po->prc = prc;
                        po->book_qty = qty;
                    }

                    if(verify_booked_order(po, side, prc, line_num)) {
                        publish(&observer::add, const_cast<order const*>(po));
                    }
                }

                auto handle_can(const char* str, uint32_t line_num) -> void {
                    auto id = extract_uint(str);
                    if(id <= 0) {
                        LOG_ERR(illegal_id, line_num);
                        return;
                    }

                    auto side = extract_side(str);
                    if(order_side::MAX == side) {
                        LOG_ERR(illegal_side, line_num);
                        return;
                    }

                    auto qty = extract_uint(str);
                    if(qty <= 0) {
                        LOG_ERR(illegal_qty, line_num);
                        return;
                    }

                    auto prc = extract_prc(str);
                    if(prc <= 0.0) {
                        LOG_ERR(illegal_prc, line_num);
                        return;
                    }

                    auto po = _orders.retrieve(id);
                    if(po->can_qty > 0) {
                        LOG_ERR(duplicated_can, line_num);
                        return;
                    }
                    if(po->qty < 0) {
                        LOG_ERR(corrupted, line_num);
                        return;
                    }

                    if(!po->qty) { // add new order according to can
                        if(!po->book_qty && !po->can_qty) {
                            LOG_WARN(can_before_add, line_num);
                            po->side = side;
                            po->prc = prc;
                        }

                        if(verify_booked_order(po, side, prc, line_num)) {
                            po->can_qty = qty;
                        }
                    }
                    else {
                        if(verify_booked_order(po, side, prc, line_num)) {
                            po->can_qty = qty;
                            publish(&observer::can, const_cast<order const*>(po), qty);
                        }
                    }
                }

                auto handle_amd(const char* str, uint32_t line_num) -> void {
                    auto id = extract_uint(str);
                    if(id <= 0) {
                        LOG_ERR(illegal_id, line_num);
                        return;
                    }

                    auto side = extract_side(str);
                    if(order_side::MAX == side) {
                        LOG_ERR(illegal_side, line_num);
                        return;
                    }

                    auto qty = extract_uint(str);
                    if(qty < 0) {
                        LOG_ERR(illegal_qty, line_num);
                        return;
                    }

                    auto prc = extract_prc(str);
                    if(prc <= 0.0) {
                        LOG_ERR(illegal_prc, line_num);
                        return;
                    }

                    auto po = _orders.retrieve(id);
                    if(po->qty < 0) {
                        LOG_ERR(corrupted, line_num);
                        return;
                    }

                    if(!po->qty) {
                        if(!po->book_qty && !po->can_qty) {
                            LOG_WARN(amd_before_add, line_num);
                            po->side = side;
                            po->prc = prc;
                        }

                        if(verify_booked_order(po, side, prc, line_num)) {
                            po->book_qty = po->book_qty > 0 ? std::min(qty, po->book_qty) : qty;
                        }
                    }
                    else {
                        if(verify_booked_order(po, side, prc, line_num)) {
                            auto old_book = po->book_qty;
                            po->book_qty = po->book_qty > 0 ? std::min(qty, po->book_qty) : qty;
                            publish(&observer::amd, const_cast<order const*>(po), old_book);
                        }
                    }
                }

                auto handle_exe(const char* str, uint32_t line_num) -> void {
                    auto iid = extract_uint(str);
                    if(iid <= 0) {
                        LOG_ERR(illegal_iid, line_num);
                        return;
                    }

                    auto exe_qty = extract_uint(str);
                    if(exe_qty <= 0) {
                        LOG_ERR(illegal_qty, line_num);
                        return;
                    }

                    auto exe_prc = extract_prc(str);
                    if(exe_prc <= 0.0) {
                        LOG_ERR(illegal_prc, line_num);
                        return;
                    }

                    auto pt = _trades.create(_tid ++);
                    pt->iid = iid;
                    pt->qty = exe_qty;
                    pt->prc = exe_prc;
                    pt->side = order_side::MAX;

                    publish(&observer::exe, const_cast<trade const*>(pt));
                }

                auto handle_comment(const char* str, uint32_t line_num) -> void {
                    if(!_log_comment) {
                        return;
                    }
                    log::info("    COMMENT -\t", line_num, "\t-" , str);
                }

                auto verify_booked_order(order* po, order_side side, double prc, int32_t line_num) -> bool {
                    if(_tolerant) {
                        return true;
                    }

                    if(po->side != side) {
                        LOG_ERR(inconsistent_side, line_num);
                        po->qty = -1;
                        return false;
                    }

                    if(std::fabs(prc - po->prc) > 0.000001) {
                        LOG_ERR(inconsistent_prc, line_num);
                        po->qty = -1;
                        return false;
                    }

                    return true;
                }

            private:
                std::string _pathname;
                bool _tolerant;
                bool _log_comment;

                bool _stop;
                std::thread _thrd;

                trade::id_type _tid = 1;

                order_container _orders;
                trade_container _trades;
        };
    }
}
