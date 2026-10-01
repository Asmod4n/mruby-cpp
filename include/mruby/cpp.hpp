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
#include <experimental/scope>
#include <expected>
#include <memory>
#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

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
        std::mutex lock;
        std::vector<mrb_value> released;
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
        {
            const std::scoped_lock hold(shared->lock);
            for (mrb_value *const cell : shared->roots)
                *cell = mrb_nil_value();
            shared->roots.clear();
            shared->released.clear();
        }
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
    std::vector<mrb_value> released;
    {
        const std::scoped_lock hold(shared->lock);
        released = shared->released;
        shared->released.clear();
    }
    for (const mrb_value v : released)
        mrb_gc_unregister(shared->mrb, v);
    std::unique_ptr<mrb_value> cell = std::make_unique<mrb_value>(value);
    const mrb_value registered = *cell;
    if (!protect(shared->mrb, [registered](mrb_state *const mrb) {
            mrb_gc_register(mrb, registered);
            return mrb_nil_value();
        })) [[unlikely]]
        throw std::bad_alloc();
    try {
        const std::scoped_lock hold(shared->lock);
        shared->roots.insert(cell.get());
        shared->released.reserve(shared->released.size() + shared->roots.size());
    } catch (const std::bad_alloc &) {
        {
            const std::scoped_lock hold(shared->lock);
            shared->roots.erase(cell.get());
        }
        mrb_gc_unregister(shared->mrb, registered);
        throw;
    }
    return std::shared_ptr<const mrb_value>(
        cell.release(), [watched = std::weak_ptr<state::owned>(shared)](const mrb_value *const p) {
            if (const std::shared_ptr<state::owned> alive = watched.lock(); alive != nullptr) {
                const std::scoped_lock hold(alive->lock);
                if (alive->roots.erase(const_cast<mrb_value *>(p)) != 0)
                    alive->released.push_back(*p);
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

class instance_variable
{
    mrb_state *mrb;
    mrb_value holder;
    mrb_sym name;

    void set(const mrb_value value)
    {
        RBasic *const object = mrb_basic_ptr(holder);
        const bool was = mrb_frozen_p(object);
        object->frozen = 0;
        const std::experimental::scope_exit refreeze([object, was] { object->frozen = was; });
        mrb_iv_set(mrb, holder, name, value);
    }
    mrb_value list()
    {
        const mrb_value held = mrb_iv_get(mrb, holder, name);
        if (mrb_array_p(held))
            return held;
        const mrb_value made = mrb_ary_new(mrb);
        set(made);
        return made;
    }

    static bool has_instance_variables(const mrb_value owner)
    {
        constexpr std::array holders{MRB_TT_OBJECT, MRB_TT_CLASS, MRB_TT_MODULE,   MRB_TT_SCLASS,
                                     MRB_TT_HASH,   MRB_TT_CDATA, MRB_TT_EXCEPTION};
        return !mrb_immediate_p(owner) && std::ranges::contains(holders, mrb_type(owner));
    }

  public:
    instance_variable(mrb_state *const given, const mrb_value owner, const std::string_view field)
        : mrb(given), holder(owner),
          name(has_instance_variables(owner)
                   ? mrb_intern_cstr(given, ("__" + std::string(field) + "__").c_str())
                   : throw std::logic_error("mruby::instance_variable keeps a value only on an "
                                            "object that has instance variables"))
    {
    }
    void assign(const mrb_value value)
    {
        set(value);
    }
    void push_back(const mrb_value value)
    {
        mrb_ary_push(mrb, list(), value);
    }
    void erase(const mrb_value value)
    {
        const mrb_value held = mrb_iv_get(mrb, holder, name);
        if (!mrb_array_p(held))
            return;
        const std::span<const mrb_value> values(RARRAY_PTR(held),
                                                static_cast<std::size_t>(RARRAY_LEN(held)));
        const auto found = std::ranges::find_if(
            values, [&](const mrb_value v) { return mrb_obj_eq(mrb, v, value); });
        if (found != values.end())
            mrb_ary_splice(mrb, held, static_cast<mrb_int>(std::distance(values.begin(), found)), 1,
                           mrb_undef_value());
    }
    void clear()
    {
        RBasic *const object = mrb_basic_ptr(holder);
        const bool was = mrb_frozen_p(object);
        object->frozen = 0;
        const std::experimental::scope_exit refreeze([object, was] { object->frozen = was; });
        mrb_iv_remove(mrb, holder, name);
    }
};

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
