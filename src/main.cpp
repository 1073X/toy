
#include <memory>
#include <iostream>

#include <signal.h>

#include "config.hpp"
#include "log.hpp"
#include "./feed/feeder_file.hpp"
#include "./order_book/manager.hpp"

using namespace toy;
using feed::feeder;

static bool _terminate = false;
auto SIGTERM_handler(int sig) -> void {
    _terminate = true;
}

auto init_log(config const& cfg) {
    int32_t sev;
    if(cfg.try_get("log_severity", sev)) {
        log::init(sev);
    }
}

auto make_feeder(config const& cfg) {
    std::string ffile;
    if(!cfg.try_get("feeder_file", ffile)) {
        log::error("invalid feeder_file");
        return (feed::feeder_file*)(nullptr);
    }

    bool tolerant;
    if(!cfg.try_get("feeder_tolerant", tolerant)) {
        tolerant = true;
    }

    bool log_comment;
    if(!cfg.try_get("feeder_log_comment", log_comment)) {
        log_comment = false;
    }

    return new feed::feeder_file(ffile, tolerant, log_comment);
}

auto make_order_book(config const& cfg) {
    int32_t lev;
    if(!cfg.try_get("order_book_level", lev)) {
        lev = 5;
    }
    if(lev <= 0 || lev > 10) {
        log::error("order_book_level must be in range [1 - 10]");
        return (order_book::manager*)nullptr;
    }

    int32_t interval;
    if(!cfg.try_get("order_book_interval", interval)) {
        interval = 10;
    }
    if(interval <= 0) {
        log::error("order_book_interval must be greater than 0");
        return (order_book::manager*)nullptr;
    }

    int32_t tolerance;
    if(!cfg.try_get("order_book_tolerance", tolerance)) {
        tolerance = 10;
    }
    if(tolerance < 0) {
        log::error("order_book_tolerance must be greater equal to 0");
        return (order_book::manager*)nullptr;
    }

    return new order_book::manager(lev, interval, tolerance);
}

auto main(int32_t argc, char** argv) -> int32_t {
    if(argc < 2) {
        log::error("invalid config file");
        return 1;
    }

    log::info("+++++++++++++++", argv[0], "started +++++++++++++");

    if(SIG_ERR == signal(SIGTERM, SIGTERM_handler)) {
        log::error("failed to install SIGTERM handler");
        return 1;
    }

    config cfg(argv[1]);
    init_log(cfg);
    std::unique_ptr<feed::feeder> pfeeder(make_feeder(cfg));
    if(!pfeeder) {
        return 1;
    }

    std::unique_ptr<order_book::manager> pbook(make_order_book(cfg));
    if(!pbook) {
        return 1;
    }

    pfeeder->register_observer(pbook.get());

    pfeeder->start();

    while(!_terminate) {
        // log.flush() ....
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    pfeeder->stop();

    log::info("---------------", argv[0], "stopped ---------------");

    return 0;
}
