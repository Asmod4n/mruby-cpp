#pragma once
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/error.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/throw.h>
#include <mruby/value.h>
#include <mruby/variable.h>

#include <cstddef>
#include <exception>
#include <expected>
#include <experimental/scope>
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

inline std::string_view string_bytes(const mrb_value string)
{
    return {RSTRING_PTR(string), static_cast<std::size_t>(RSTRING_LEN(string))};
}

inline std::size_t array_length(const mrb_value array)
{
    return static_cast<std::size_t>(RARRAY_LEN(array));
}

inline std::string_view class_name(mrb_state *const mrb, const mrb_value klass)
{
    return mrb_class_name(mrb, mrb_class_ptr(klass));
}


struct lifetime {
    bool alive = true;
};

inline void lifetime_end(mrb_state *, void *const held)
{
    const std::unique_ptr<std::shared_ptr<lifetime>> owned(static_cast<std::shared_ptr<lifetime> *>(held));
    if (owned != nullptr)
        (*owned)->alive = false;
}

inline constexpr mrb_data_type wrapper{"mruby-cpp", lifetime_end};

class state;

class handle
{
    std::shared_ptr<const lifetime> life;
    mrb_state *mrb;
    mrb_value object;

  public:
    handle(std::shared_ptr<const lifetime> given_life, mrb_state *const given_mrb, const mrb_value given)
        : life(std::move(given_life)), mrb(given_mrb), object(given)
    {
    }
    mrb_state *state() const
    {
        if (!life->alive) [[unlikely]]
            throw std::logic_error("an mruby object is used after mruby freed it");
        return mrb;
    }
    mrb_value value() const
    {
        (void)state();
        return object;
    }
};

class RString
{
    handle held;
    friend class state;
    explicit RString(handle given) : held(std::move(given))
    {
    }

  public:
    std::string bytes() &
    {
        return std::string(string_bytes(held.value()));
    }
    std::string_view bytes() const &
    {
        return string_bytes(held.value());
    }
};

class RArray
{
    handle held;
    friend class state;
    explicit RArray(handle given) : held(std::move(given))
    {
    }

  public:
    std::size_t size() const
    {
        return array_length(held.value());
    }
};

class RHash
{
    handle held;
    friend class state;
    explicit RHash(handle given) : held(std::move(given))
    {
    }

  public:
    std::size_t size() const
    {
        mrb_state *const mrb = held.state();
        return static_cast<std::size_t>(mrb_hash_size(mrb, held.value()));
    }
};

class RClass
{
    handle held;
    friend class state;
    explicit RClass(handle given) : held(std::move(given))
    {
    }

  public:
    std::string_view name() const
    {
        mrb_state *const mrb = held.state();
        return class_name(mrb, held.value());
    }
};

class state
{
    struct owned {
        mrb_state *mrb = nullptr;
        bool closes = true;
        std::thread::id owner = std::this_thread::get_id();
        std::unordered_set<mrb_value *> roots;
    };
    std::shared_ptr<owned> shared = std::make_shared<owned>();

    template <class R, class F> R wrap(F &&make)
    {
        mrb_state *const mrb = shared->mrb;
        const int arena = mrb_gc_arena_save(mrb);
        std::experimental::scope_exit restore([mrb, arena] { mrb_gc_arena_restore(mrb, arena); });
        const mrb_value object = make(mrb);
        auto held = std::make_unique<std::shared_ptr<lifetime>>(std::make_shared<lifetime>());
        std::shared_ptr<const lifetime> life = *held;
        RData *const data = mrb_data_object_alloc(mrb, mrb->object_class, held.get(), &wrapper);
        held.release();
        const mrb_value carrier = mrb_obj_value(data);
        mrb_iv_set(mrb, carrier, mrb_intern_lit(mrb, "__object__"), object);
        restore.release();
        mrb_gc_arena_restore(mrb, arena);
        mrb_gc_protect(mrb, carrier);
        return R(handle(std::move(life), mrb, object));
    }

  public:
    state()
    {
        shared->mrb = mrb_open();
        if (shared->mrb == nullptr) [[unlikely]]
            throw std::bad_alloc();
    }
    explicit state(mrb_state *const given)
    {
        shared->mrb = given;
        shared->closes = false;
    }
    state(const state &) = delete;
    state &operator=(const state &) = delete;
    ~state()
    {
        for (mrb_value *const cell : shared->roots) {
            if (!shared->closes)
                mrb_gc_unregister(shared->mrb, *cell);
            *cell = mrb_undef_value();
        }
        shared->roots.clear();
        if (shared->closes)
            mrb_close(shared->mrb);
        shared->mrb = nullptr;
    }
    RString str_new(const std::string_view bytes)
    {
        return wrap<RString>([bytes](mrb_state *const mrb) {
            return mrb_str_new(mrb, bytes.data(), static_cast<mrb_int>(bytes.size()));
        });
    }
    RArray ary_new()
    {
        return wrap<RArray>([](mrb_state *const mrb) { return mrb_ary_new(mrb); });
    }
    RHash hash_new()
    {
        return wrap<RHash>([](mrb_state *const mrb) { return mrb_hash_new(mrb); });
    }
    RClass define_class(const std::string &name)
    {
        return wrap<RClass>([&name](mrb_state *const mrb) {
            return mrb_obj_value(mrb_define_class(mrb, name.c_str(), mrb->object_class));
        });
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
