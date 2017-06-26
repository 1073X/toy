#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

#include "log.hpp"

namespace toy {
    class config {
        public:
            config(std::string const& pathname) {
                std::ifstream s(pathname, std::ios_base::in);
                if(!s.good()) {
                    throw std::runtime_error("invalid config file");
                }

                while(!s.eof()) {
                    std::string line; std::getline(s, line);
                    
                    if(line.empty() || '#' == line[0]) {
                        continue;
                    }

                    std::istringstream ss(line);
                    std::string key; std::getline(ss, key, '=');
                    std::string val; std::getline(ss, val);

                    if(!_items.insert({ key, val }).second) {
                        throw std::runtime_error("duplicated config item " + key);
                    }
                }
            }

            template<typename T> auto try_get(std::string const& key, T& val) const {
                auto it = _items.find(key);
                if(_items.end() == it) {
                    return false;
                }

                if(cast(it->second, val)) {
                    log::debug(key, val);
                    return true;
                }

                log::warn("missing", key);
                return false;
            }

        private:
            auto cast(std::string const& raw, std::string& val) const {
                val = raw;
                return true;
            }

            auto cast(std::string const& raw, int32_t& val) const {
                val = std::atoi(raw.c_str());
                return true;
            }

            auto cast(std::string const& raw, bool& val) const {
                std::string lower(raw.size(), ' ');
                std::transform(raw.begin(), raw.end(), lower.begin(), ::tolower);
                if(lower == "true" || lower == "yes" || lower == "1") {
                    val = true;
                    return true;
                }
                else if(lower == "false" || lower == "no" || lower == "0") {
                    val = false;
                    return true;
                }
                return false;
            }

        private:
            std::map<std::string, std::string> _items;
    };
}
