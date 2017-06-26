#pragma once

#include "reference/market.hpp"

#include "book.hpp"

namespace toy {
    namespace order_book {

        using market_entity = reference::market;
        using book_entity = book;

        class instrument {
            public:
                using id_type = instrument_id;
                static id_type const invalid_id = (instrument_id)-1;
                id_type id = invalid_id;

            public:
                instrument() = default;
                instrument(instrument_id id, int32_t max_lev)
                    : id(id), _pbook(new book_entity(id)), _pmkt(new market_entity(id, max_lev)) {}

                instrument(instrument const&) = delete;
                auto operator=(instrument const&) = delete;

                instrument(instrument&& a) : instrument() {
                    operator=(std::move(a));
                }

                auto operator=(instrument&& a) -> instrument& {
                    std::swap(_pbook, a._pbook);
                    std::swap(_pmkt, a._pmkt);
                    std::swap(id, a.id);
                    return *this;
                }

                auto book() { return _pbook.get(); }
                auto market() { return _pmkt.get(); }


            private:
                std::unique_ptr<book_entity> _pbook { nullptr };
                std::unique_ptr<market_entity> _pmkt { nullptr };
        };

    }
}
