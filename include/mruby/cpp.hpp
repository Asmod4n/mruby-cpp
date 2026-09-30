#pragma once
#include <mruby.h>
#include <mruby/value.h>

#include <stdexcept>
#include <thread>

namespace mruby {

class value {
    mrb_value object;
    std::thread::id owner = std::this_thread::get_id();
    int locks = 0;
    bool thaws = false;

public:
    explicit value(const mrb_value given) : object(given) {}
    value(const value &) = delete;
    value &operator=(const value &) = delete;
    void lock()
    {
        if (std::this_thread::get_id() != owner) [[unlikely]]
            throw std::logic_error("a thread tries to lock an mruby::value of an mrb_state that another thread owns");
        if (mrb_immediate_p(object) || locks++ > 0) return;
        RBasic *const basic = mrb_basic_ptr(object);
        thaws = !mrb_frozen_p(basic);
        basic->frozen = 1;
    }
    void unlock()
    {
        if (mrb_immediate_p(object) || --locks > 0) return;
        if (thaws) mrb_basic_ptr(object)->frozen = 0;
        thaws = false;
    }
    operator mrb_value() const { return object; }
};

}
