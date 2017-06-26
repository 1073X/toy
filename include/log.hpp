#pragma once

#include <iostream>
#include <mutex>

#include "reference/to_string.hpp"

namespace toy {

    class log {
        private:
            enum struct severity {
                debug = 0, info, warn, error, MAX
            };

        private:
            log() {}

            auto print(std::ostream& s) {
                s << std::endl;
            }

            template<typename T, typename ... ARGS> auto print(std::ostream& s, T const& t, ARGS ... args) {
                s << ' ' << t;
                print(s, args ...);
            }

            template<typename ... ARGS> auto print(severity sev, ARGS ... args) {
                if(sev < _severity) {
                    return;
                }

                std::lock_guard<decltype(_mtx)> l(_mtx);
                switch(sev) {
                    case severity::debug: print(std::cout, "[DEBUG]", args ...); break;
                    case severity::info: print(std::cout, "[ INFO]", args ...); break;
                    case severity::warn: print(std::cout, "[ WARN]", args ...); break;
                    case severity::error: print(std::cerr, "[ERROR]", args ...); break;
                    default: break;
                }
            }

        public:
            ~log() {}

            static auto init(int32_t sev) {
                _instance._severity = (severity)sev;
            }

            template<typename ... ARGS> static auto debug(ARGS ... args) {
                _instance.print(severity::debug, args ...);
            }

            template<typename ... ARGS> static auto info(ARGS ... args) {
                _instance.print(severity::info, args ...);
            }

            template<typename ... ARGS> static auto warn(ARGS ... args) {
                _instance.print(severity::warn, args ...);
            }

            template<typename ... ARGS> static auto error(ARGS ... args) {
                _instance.print(severity::error, args ...);
            }

        private:
            static log _instance;

            severity _severity = severity::debug;
            std::mutex _mtx;
    };

}

using toy::log;
