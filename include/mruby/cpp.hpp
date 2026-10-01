#pragma once
#include <mruby.h>
#include <mruby/value.h>

#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

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

class automatic {
    mrb_value object;

public:
    explicit automatic(const mrb_value given) : object(given) {}
    automatic(const automatic &) = delete;
    automatic &operator=(const automatic &) = delete;
    static void *operator new(std::size_t) = delete;
    static void *operator new[](std::size_t) = delete;
    operator mrb_value() const { return object; }
};

struct life {
    mrb_state *mrb = nullptr;
    std::thread::id owner = std::this_thread::get_id();
    std::mutex lock;
    std::vector<mrb_value> released;
    std::unordered_set<mrb_value *> roots;
};

class state {
    std::shared_ptr<life> shared = std::make_shared<life>();

public:
    state()
    {
        shared->mrb = mrb_open();
        if (shared->mrb == nullptr) [[unlikely]] throw std::bad_alloc();
    }
    state(const state &) = delete;
    state &operator=(const state &) = delete;
    ~state()
    {
        {
            const std::scoped_lock hold(shared->lock);
            for (mrb_value *const cell : shared->roots) *cell = mrb_nil_value();
            shared->roots.clear();
            shared->released.clear();
        }
        mrb_close(shared->mrb);
        shared->mrb = nullptr;
    }
    mrb_state *get() const { return shared->mrb; }
    friend std::shared_ptr<const mrb_value> root(const state &owner, const automatic &value);
};

inline std::shared_ptr<const mrb_value> root(const state &owner, const automatic &value)
{
    const std::shared_ptr<life> &shared = owner.shared;
    if (std::this_thread::get_id() != shared->owner) [[unlikely]]
        throw std::logic_error("a thread tries to root a value of an mrb_state that another thread owns");
    std::vector<mrb_value> released;
    {
        const std::scoped_lock hold(shared->lock);
        released.swap(shared->released);
    }
    for (const mrb_value v : released) mrb_gc_unregister(shared->mrb, v);
    std::unique_ptr<mrb_value> cell = std::make_unique<mrb_value>(value);
    mrb_gc_register(shared->mrb, *cell);
    try {
        const std::scoped_lock hold(shared->lock);
        shared->roots.insert(cell.get());
    } catch (const std::bad_alloc &) {
        mrb_gc_unregister(shared->mrb, *cell);
        throw;
    }
    return std::shared_ptr<const mrb_value>(cell.release(), [shared](const mrb_value *const p) {
        {
            const std::scoped_lock hold(shared->lock);
            if (shared->roots.erase(const_cast<mrb_value *>(p)) != 0) shared->released.push_back(*p);
        }
        delete p;
    });
}

}

