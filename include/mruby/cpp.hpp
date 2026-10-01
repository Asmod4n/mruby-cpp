#pragma once
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/error.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/throw.h>
#include <mruby/value.h>
#include <mruby/variable.h>

#include <cstddef>
#include <exception>
#include <expected>
#include <memory>
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <new>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace mruby
{

class automatic;

class frozen
{
    RBasic *object = nullptr;
    RBasic *singleton = nullptr;
    bool thaws = false;

    void freeze(mrb_state *const mrb, const mrb_value given)
    {
        if (mrb_immediate_p(given))
            return;
        object = mrb_basic_ptr(given);
        thaws = !mrb_frozen_p(object);
        if (thaws && object->c->tt == MRB_TT_SCLASS && !mrb_frozen_p(object->c))
            singleton = reinterpret_cast<RBasic *>(object->c);
        mrb_obj_freeze(mrb, given);
    }

  public:
    frozen(mrb_state *const mrb, const automatic &given);
    frozen(mrb_state *const mrb, const std::shared_ptr<const mrb_value> &given)
    {
        freeze(mrb, *given);
    }
    frozen(const frozen &) = delete;
    frozen &operator=(const frozen &) = delete;
    static void *operator new(std::size_t) = delete;
    static void *operator new[](std::size_t) = delete;
    ~frozen()
    {
        if (thaws)
            object->frozen = 0;
        if (singleton != nullptr)
            singleton->frozen = 0;
    }
};

class automatic
{
    mrb_value object;

  public:
    explicit automatic(const mrb_value given) : object(given)
    {
    }
    automatic(const automatic &) = delete;
    automatic &operator=(const automatic &) = delete;
    static void *operator new(std::size_t) = delete;
    static void *operator new[](std::size_t) = delete;
    operator mrb_value() const
    {
        return object;
    }
};

inline frozen::frozen(mrb_state *const mrb, const automatic &given)
{
    freeze(mrb, given);
}

template <class F> std::expected<mrb_value, mrb_value> protect(mrb_state *const mrb, F &&body);

class state
{
    struct owned {
        mrb_state *mrb = nullptr;
        std::thread::id owner = std::this_thread::get_id();
        std::unordered_set<mrb_value *> roots;
    };
    std::shared_ptr<owned> shared = std::make_shared<owned>();

  public:
    state()
    {
        shared->mrb = mrb_open();
        if (shared->mrb == nullptr) [[unlikely]]
            throw std::bad_alloc();
    }
    state(const state &) = delete;
    state &operator=(const state &) = delete;
    ~state()
    {
        for (mrb_value *const cell : shared->roots)
            *cell = mrb_undef_value();
        shared->roots.clear();
        mrb_close(shared->mrb);
        shared->mrb = nullptr;
    }
    mrb_state *get() const
    {
        return shared->mrb;
    }
    std::shared_ptr<const mrb_value> root(const automatic &value);
};

inline std::shared_ptr<const mrb_value> state::root(const automatic &value)
{
    if (std::this_thread::get_id() != shared->owner) [[unlikely]]
        throw std::logic_error(
            "a thread tries to root a value of an mrb_state that another thread owns");
    std::unique_ptr<mrb_value> cell = std::make_unique<mrb_value>(value);
    const mrb_value registered = *cell;
    if (!protect(shared->mrb, [registered](mrb_state *const mrb) {
            mrb_gc_register(mrb, registered);
            return mrb_nil_value();
        })) [[unlikely]]
        throw std::bad_alloc();
    try {
        shared->roots.insert(cell.get());
    } catch (const std::bad_alloc &) {
        mrb_gc_unregister(shared->mrb, registered);
        throw;
    }
    return std::shared_ptr<const mrb_value>(
        cell.release(), [watched = std::weak_ptr<state::owned>(shared)](const mrb_value *const p) {
            if (const std::shared_ptr<state::owned> alive = watched.lock(); alive != nullptr) {
                alive->roots.erase(const_cast<mrb_value *>(p));
                mrb_gc_unregister(alive->mrb, *p);
            }
            delete p;
        });
}

template <class F> std::expected<mrb_value, mrb_value> protect(mrb_state *const mrb, F &&body)
{
    struct call {
        std::remove_reference_t<F> *body;
        std::exception_ptr thrown;
    };
    call made{&body, nullptr};
    mrb_bool error = false;
    const mrb_value answer = mrb_protect_error(
        mrb,
        [](mrb_state *const state, void *const given) -> mrb_value {
            call *const c = static_cast<call *>(given);
            try {
                return (*c->body)(state);
            } catch (mrb_jmpbuf *) {
                throw;
            } catch (...) {
                c->thrown = std::current_exception();
                return mrb_nil_value();
            }
        },
        &made, &error);
    if (made.thrown) [[unlikely]]
        std::rethrow_exception(made.thrown);
    if (error) [[unlikely]]
        return std::unexpected(answer);
    return answer;
}

template <std::size_t N> struct literal {
    std::array<char, N> bytes{};
    consteval literal(const char (&given)[N])
    {
        std::ranges::copy(given, bytes.begin());
    }
    constexpr std::string_view view() const
    {
        return {bytes.data(), N - 1};
    }
};

template <literal Name> mrb_sym symbol(mrb_state *const mrb)
{
    return mrb_intern_static(mrb, Name.bytes.data(), Name.view().size());
}

template <literal Bytes> mrb_value str_new_static(mrb_state *const mrb)
{
    return mrb_str_new_static(mrb, Bytes.bytes.data(), static_cast<mrb_int>(Bytes.view().size()));
}

}
