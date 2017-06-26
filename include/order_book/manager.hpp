#pragma once

#include "log.hpp"
#include "reference/order.hpp"
#include "reference/container.hpp"
#include "feed/observer.hpp"

#include "instrument.hpp"

namespace toy {
    namespace order_book {

        using reference::order;
        using reference::trade;
        using reference::market;

        class manager : public feed::observer {
            public:
                manager(int32_t max_lev, int32_t interval, int32_t tolerance)
                    : _max_lev(max_lev), _interval(interval), _tolerance(tolerance) {
                }

            private: // feed observer
                auto add(order const* po) -> void override {
                    assert(po->qty > 0);
                    assert(po->qty >= po->book_qty);
                    assert(po->qty >= po->can_qty);

                    log::debug("New", po);
                    
                    auto pinst = _instruments.retrieve(po->iid, _max_lev);
                    assert(pinst != nullptr);
                    auto pbook = pinst->book();
                    assert(pbook != nullptr);

                    auto times = pbook->add(po->side, po->qty - po->can_qty, po->prc);

                    if(!po->can_qty) {
                        update(times, pinst);
                    }
                }

                auto can(order const* po, int64_t can_qty) -> void override {
                    log::debug("Can", po, can_qty);

                    auto pinst = _instruments.find(po->iid);
                    assert(pinst != nullptr);
                    auto pbook = pinst->book();
                    auto times = pbook->can(po->side, can_qty, po->prc);
                    if(po->can_qty == po->book_qty) {
                        update(times, pinst);
                    }
                }

                auto amd(order const* po, int64_t old_book) -> void override {
                    log::debug("Amd", po, old_book, "->", po->book_qty);

                    auto pinst = _instruments.find(po->iid);
                    assert(pinst != nullptr);
                    auto pbook = pinst->book();
                    auto times = pbook->amd(po->side, old_book - po->book_qty, po->prc);

                    update(times, pinst);
                }

                auto exe(trade const* pt) -> void override {

                    auto pinst = _instruments.find(pt->iid);
                    if(!pinst) {
                        log::error("LOGIC [unknown_exe]", pt);
                        return;
                    }

                    auto pbook = pinst->book();
                    assert(pbook != nullptr);
 
                    pbook->exe(pt->qty, pt->prc);

                    log::info("Exe", pt);
                }

            private:
                auto update(int32_t times, instrument* pinst) -> void {
                    if(times < _interval) {
                        return;
                    }

                    auto pbook = pinst->book();
                    auto pmkt = pinst->market();

                    if(!pbook->verify(_tolerance / 2, _max_lev)) {
                        return;
                    }

                    if(!pbook->try_extract(pmkt)) {
                        return;
                    }
                    
                    log::info(pmkt);
                }
 
            private:
                int32_t const _max_lev;
                int32_t const _interval;
                int32_t const _tolerance;

                reference::container<instrument> _instruments;
        };

    }
}
