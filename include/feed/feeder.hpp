#pragma once

#include <vector>

#include "observer.hpp"

namespace toy {
    namespace feed {

        class feeder {
            public:
                virtual ~feeder() {}

                virtual auto start() -> bool = 0;
                virtual auto stop() -> void = 0;

                auto register_observer(observer* pob) {
                    _observers.push_back(pob);
                }

            protected:
                template<typename ... ARGS>
                auto publish(void (observer::*func)(ARGS ...), ARGS ... args) {
                    for(auto pob : _observers) {
                        (pob->*func)(args ...);
                    }
                }

            private:
                std::vector<observer*> _observers;
        };

    }
}
