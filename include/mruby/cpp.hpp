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
#include <concepts>
#include <utility>
#include <tuple>
#include <span>
#include <optional>
#include <cstdint>
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

inline bool class_p(const mrb_value value)
{
    return mrb_type(value) == MRB_TT_CLASS || mrb_type(value) == MRB_TT_MODULE ||
           mrb_type(value) == MRB_TT_SCLASS;
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

struct clock {
    std::uint64_t epoch = 0;
    bool open = true;
};

class handle
{
    std::shared_ptr<const lifetime> life;
    std::shared_ptr<clock> time;
    mrb_state *mrb;
    mrb_value object;

  public:
    handle(std::shared_ptr<const lifetime> given_life, std::shared_ptr<clock> given_time,
           mrb_state *const given_mrb, const mrb_value given)
        : life(std::move(given_life)), time(std::move(given_time)), mrb(given_mrb), object(given)
    {
    }
    mrb_state *state() const
    {
        if (!life->alive || !time->open) [[unlikely]]
            throw std::logic_error("an mruby object is used after mruby freed it");
        return mrb;
    }
    mrb_value value() const
    {
        (void)state();
        return object;
    }
    const std::shared_ptr<clock> &clock_of() const
    {
        return time;
    }
};

class borrowed
{
    std::shared_ptr<clock> time;
    std::uint64_t epoch;
    mrb_state *mrb;
    mrb_value object;
    int arena;

  public:
    borrowed(std::shared_ptr<clock> given_time, mrb_state *const given_mrb, const mrb_value given,
             const int made_at = -1)
        : time(std::move(given_time)), epoch(time->epoch), mrb(given_mrb), object(given), arena(made_at)
    {
    }
    int made_at() const
    {
        return arena;
    }
    mrb_state *state() const
    {
        if (!time->open || time->epoch != epoch) [[unlikely]]
            throw std::logic_error("an mruby view is used after Ruby ran");
        return mrb;
    }
    mrb_value value() const
    {
        (void)state();
        return object;
    }
    const std::shared_ptr<clock> &clock_of() const
    {
        return time;
    }
};

inline handle wrap(mrb_state *const mrb, std::shared_ptr<clock> time, const mrb_value object, const int arena)
{
    std::experimental::scope_exit restore([mrb, arena] { mrb_gc_arena_restore(mrb, arena); });
    mrb_gc_protect(mrb, object);
    auto held = std::make_unique<std::shared_ptr<lifetime>>(std::make_shared<lifetime>());
    std::shared_ptr<const lifetime> life = *held;
    RData *const data = mrb_data_object_alloc(mrb, mrb->object_class, held.get(), &wrapper);
    held.release();
    const mrb_value carrier = mrb_obj_value(data);
    mrb_iv_set(mrb, carrier, mrb_intern_lit(mrb, "__object__"), object);
    restore.release();
    mrb_gc_arena_restore(mrb, arena);
    mrb_gc_protect(mrb, carrier);
    return handle(std::move(life), std::move(time), mrb, object);
}

inline handle wrap(mrb_state *const mrb, std::shared_ptr<clock> time, const mrb_value object)
{
    return wrap(mrb, std::move(time), object, mrb_gc_arena_save(mrb));
}

class state;
class RArray;
class RHash;
class value;
struct access;
template <class T> struct answer;
template <class T, class Key>
std::optional<typename answer<T>::type> hash_get(const std::shared_ptr<clock> &time, mrb_state *mrb,
                                                 mrb_value hash, const Key &key);

class RBasic
{
  protected:
    handle held;
    explicit RBasic(handle given) : held(std::move(given))
    {
    }

  public:
    static bool type_p(const mrb_value given)
    {
        return !mrb_immediate_p(given);
    }

    class view
    {
      protected:
        borrowed seen;
        explicit view(borrowed given) : seen(std::move(given))
        {
        }
        friend class RBasic;
        friend class value;
        friend class RArray;
        friend class RHash;
        friend class state;
        friend struct access;
        mrb_value value_of() const
        {
            return seen.value();
        }
    };

  protected:
    mrb_value value_of() const
    {
        return held.value();
    }
    friend class state;
    friend struct access;
};

class RString : public RBasic
{
    friend class state;
    using RBasic::RBasic;

  public:
    static bool type_p(const mrb_value given)
    {
        return mrb_string_p(given);
    }

    class view : public RBasic::view
    {
        using RBasic::view::view;
        friend class RString;
        friend class RArray;
        friend class RHash;
        friend class state;
        friend struct access;

      public:
        std::string_view bytes() const &
        {
            return string_bytes(seen.value());
        }
    };

    explicit RString(const view &from) : RBasic(wrap(from.seen.state(), from.seen.clock_of(), from.seen.value()))
    {
    }
    std::string bytes() &
    {
        return std::string(string_bytes(held.value()));
    }
    std::string_view bytes() const &
    {
        return string_bytes(held.value());
    }
};

template <class T>
concept object_type = requires(const mrb_value given) {
    { T::type_p(given) } -> std::same_as<bool>;
    typename T::view;
};

template <class T>
concept scalar = std::same_as<T, mrb_int> || std::same_as<T, mrb_float> || std::same_as<T, bool>;

template <object_type T> struct answer<T> {
    using type = typename T::view;
};
template <scalar T> struct answer<T> {
    using type = T;
};

struct access {
    template <class T>
    static std::optional<typename answer<T>::type> read(const std::shared_ptr<clock> &time, mrb_state *const mrb,
                                                        const mrb_value given)
    {
        if constexpr (object_type<T>) {
            if (!T::type_p(given))
                return std::nullopt;
            return typename T::view(borrowed(time, mrb, given));
        } else if constexpr (std::same_as<T, mrb_int>) {
            if (!mrb_integer_p(given))
                return std::nullopt;
            return mrb_integer(given);
        } else if constexpr (std::same_as<T, mrb_float>) {
            if (!mrb_float_p(given))
                return std::nullopt;
            return mrb_float(given);
        } else {
            if (!mrb_true_p(given) && !mrb_false_p(given))
                return std::nullopt;
            return mrb_true_p(given);
        }
    }
    static const borrowed &seen_of(const RBasic::view &given)
    {
        return given.seen;
    }
    template <class T> static mrb_value raw(const T &given)
    {
        return given.value_of();
    }
};

class value
{
    borrowed seen;
    friend class state;
    friend class RArray;
    friend class RHash;
    explicit value(borrowed given) : seen(std::move(given))
    {
    }

  public:
    value(const RBasic::view &given) : seen(access::seen_of(given))
    {
    }
    template <class T> std::optional<typename answer<T>::type> as() const
    {
        return access::read<T>(seen.clock_of(), seen.state(), seen.value());
    }
    bool nil_p() const
    {
        return mrb_nil_p(seen.value());
    }

  private:
    friend struct access;
    mrb_value value_of() const
    {
        return seen.value();
    }
};

inline int attached_below(const int top, const borrowed &child)
{
    return child.made_at() >= 0 && top == child.made_at() + 1 ? child.made_at() : -1;
}

inline int attached_below(const int top, const borrowed &key, const borrowed &child)
{
    if (key.made_at() < 0)
        return attached_below(top, child);
    if (child.made_at() < 0)
        return attached_below(top, key);
    const int low = std::min(key.made_at(), child.made_at());
    const int high = std::max(key.made_at(), child.made_at());
    return high == low + 1 && top == high + 1 ? low : -1;
}

inline void attach_done(mrb_state *const mrb, const int below)
{
    if (below >= 0)
        mrb_gc_arena_restore(mrb, below);
}

class RArray : public RBasic
{
    friend class state;
    using RBasic::RBasic;

    template <class T>
    static std::optional<typename answer<T>::type> element(const std::shared_ptr<clock> &time,
                                                           mrb_state *const mrb, const mrb_value array,
                                                           const std::size_t index)
    {
        if (index >= array_length(array))
            return std::nullopt;
        return access::read<T>(time, mrb, mrb_ary_entry(array, static_cast<mrb_int>(index)));
    }

  public:
    static bool type_p(const mrb_value given)
    {
        return mrb_array_p(given);
    }

    class view : public RBasic::view
    {
        using RBasic::view::view;
        friend class RArray;
        friend class RHash;
        friend class state;
        friend struct access;

      public:
        std::size_t size() const
        {
            return array_length(seen.value());
        }
        template <class T> std::optional<typename answer<T>::type> at(const std::size_t index) const
        {
            return element<T>(seen.clock_of(), seen.state(), seen.value(), index);
        }
        void push(const value &child) const
        {
            mrb_state *const mrb = seen.state();
            const int below = attached_below(mrb_gc_arena_save(mrb), child.seen);
            mrb_ary_push(mrb, seen.value(), child.seen.value());
            attach_done(mrb, below);
        }
    };

    explicit RArray(const view &from) : RBasic(wrap(from.seen.state(), from.seen.clock_of(), from.seen.value()))
    {
    }
    std::size_t size() const
    {
        return array_length(held.value());
    }
    template <class T> std::optional<typename answer<T>::type> at(const std::size_t index) const
    {
        return element<T>(held.clock_of(), held.state(), held.value(), index);
    }
    void push(const value &child) const
    {
        mrb_state *const mrb = held.state();
        const int below = attached_below(mrb_gc_arena_save(mrb), child.seen);
        mrb_ary_push(mrb, held.value(), child.seen.value());
        attach_done(mrb, below);
    }
};

inline mrb_value key_of(mrb_state *const mrb, const mrb_int given)
{
    return mrb_int_value(mrb, given);
}
inline mrb_value key_of(mrb_state *const mrb, const mrb_float given)
{
    return mrb_float_value(mrb, given);
}
template <class T>
    requires std::same_as<T, RString> || std::same_as<T, RString::view>
mrb_value key_of(mrb_state *, const T &given)
{
    return access::raw(given);
}

class RHash : public RBasic
{
    friend class state;
    using RBasic::RBasic;

  public:
    static bool type_p(const mrb_value given)
    {
        return mrb_hash_p(given);
    }

    class view : public RBasic::view
    {
        using RBasic::view::view;
        friend class RHash;
        friend class RArray;
        friend class state;
        friend struct access;

      public:
        std::size_t size() const
        {
            mrb_state *const mrb = seen.state();
            return static_cast<std::size_t>(mrb_hash_size(mrb, seen.value()));
        }
        template <class T, class Key> std::optional<typename answer<T>::type> get(const Key &key) const
        {
            return hash_get<T>(seen.clock_of(), seen.state(), seen.value(), key);
        }
        template <class Key> void set(const Key &key, const value &child) const
        {
            mrb_state *const mrb = seen.state();
            int below = -1;
            if constexpr (std::derived_from<Key, RBasic::view>)
                below = attached_below(mrb_gc_arena_save(mrb), access::seen_of(key), child.seen);
            else
                below = attached_below(mrb_gc_arena_save(mrb), child.seen);
            mrb_hash_set(mrb, seen.value(), key_of(mrb, key), child.seen.value());
            attach_done(mrb, below);
        }
    };

    explicit RHash(const view &from) : RBasic(wrap(from.seen.state(), from.seen.clock_of(), from.seen.value()))
    {
    }
    std::size_t size() const
    {
        mrb_state *const mrb = held.state();
        return static_cast<std::size_t>(mrb_hash_size(mrb, held.value()));
    }
    template <class T, class Key> std::optional<typename answer<T>::type> get(const Key &key) const
    {
        return hash_get<T>(held.clock_of(), held.state(), held.value(), key);
    }
    template <class Key> void set(const Key &key, const value &child) const
    {
        mrb_state *const mrb = held.state();
        int below = -1;
        if constexpr (std::derived_from<Key, RBasic::view>)
            below = attached_below(mrb_gc_arena_save(mrb), access::seen_of(key), child.seen);
        else
            below = attached_below(mrb_gc_arena_save(mrb), child.seen);
        mrb_hash_set(mrb, held.value(), key_of(mrb, key), child.seen.value());
        attach_done(mrb, below);
    }
};

class RClass : public RBasic
{
    friend class state;
    using RBasic::RBasic;

  public:
    static bool type_p(const mrb_value given)
    {
        return class_p(given);
    }

    class view : public RBasic::view
    {
        using RBasic::view::view;
        friend class RClass;
        friend class RArray;
        friend class RHash;
        friend class state;
        friend struct access;

      public:
        std::string_view name() const
        {
            mrb_state *const mrb = seen.state();
            return class_name(mrb, seen.value());
        }
    };

    explicit RClass(const view &from) : RBasic(wrap(from.seen.state(), from.seen.clock_of(), from.seen.value()))
    {
    }
    std::string_view name() const
    {
        mrb_state *const mrb = held.state();
        return class_name(mrb, held.value());
    }
};

template <class T>
concept view_type = std::derived_from<T, RBasic::view> || std::same_as<T, value>;

template <class T, class Key>
std::optional<typename answer<T>::type> hash_get(const std::shared_ptr<clock> &time, mrb_state *const mrb,
                                                 const mrb_value hash, const Key &key)
{
    const mrb_value found = mrb_hash_fetch(mrb, hash, key_of(mrb, key), mrb_undef_value());
    if (mrb_undef_p(found))
        return std::nullopt;
    return access::read<T>(time, mrb, found);
}

template <class F> struct parameters_of : parameters_of<decltype(&F::operator())> {
};
template <class C, class R, class... P> struct parameters_of<R (C::*)(P...) const> {
    using result = R;
    using types = std::tuple<std::remove_cvref_t<P>...>;
};
template <class C, class R, class... P> struct parameters_of<R (C::*)(P...)> {
    using result = R;
    using types = std::tuple<std::remove_cvref_t<P>...>;
};

template <class T> struct viewed;
template <class T>
    requires requires { typename T::view; }
struct viewed<T> {
    using type = T;
};
template <> struct viewed<RString::view> {
    using type = RString;
};
template <> struct viewed<RArray::view> {
    using type = RArray;
};
template <> struct viewed<RHash::view> {
    using type = RHash;
};
template <> struct viewed<RClass::view> {
    using type = RClass;
};
template <scalar T> struct viewed<T> {
    using type = T;
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
    std::shared_ptr<clock> time = std::make_shared<clock>();

    template <class R, class F> R make(F &&made)
    {
        mrb_state *const mrb = shared->mrb;
        const int arena = mrb_gc_arena_save(mrb);
        std::experimental::scope_exit restore([mrb, arena] { mrb_gc_arena_restore(mrb, arena); });
        const mrb_value object = made(mrb);
        restore.release();
        return R(wrap(mrb, time, object, arena));
    }

    mrb_value argument(const mrb_int given) const
    {
        return mrb_int_value(shared->mrb, given);
    }
    mrb_value argument(const mrb_float given) const
    {
        return mrb_float_value(shared->mrb, given);
    }
    mrb_value argument(const bool given) const
    {
        return mrb_bool_value(given);
    }
    mrb_value argument(std::nullptr_t) const
    {
        return mrb_nil_value();
    }
    template <object_type T> mrb_value argument(const T &given) const
    {
        return access::raw(given);
    }
    template <view_type V> mrb_value argument(const V &given) const
    {
        return access::raw(given);
    }

    template <class F> struct block {
        const F *body;
        std::exception_ptr thrown;
        std::shared_ptr<clock> time;
    };

    template <class F> static mrb_value block_call(mrb_state *const mrb, mrb_value)
    {
        const mrb_value env = mrb_proc_cfunc_env_get(mrb, 0);
        const mrb_value carrier = mrb_proc_cfunc_env_get(mrb, 1);
        RData *const data = static_cast<RData *>(mrb_ptr(carrier));
        if (data->type != &wrapper || !(*static_cast<std::shared_ptr<lifetime> *>(data->data))->alive)
            [[unlikely]]
            mrb_raise(mrb, E_RUNTIME_ERROR, "a C++ block is called after the call that gave it returned");
        block<F> *const call = static_cast<block<F> *>(mrb_cptr(env));
        ++call->time->epoch;
        const mrb_value *argv = nullptr;
        mrb_int argc = 0;
        mrb_get_args(mrb, "*", &argv, &argc);
        using types = typename parameters_of<F>::types;
        constexpr std::size_t count = std::tuple_size_v<types>;
        if (static_cast<std::size_t>(argc) != count) [[unlikely]]
            mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (given %i, expected %i)", argc,
                       static_cast<mrb_int>(count));
        const std::span<const mrb_value> given(argv, static_cast<std::size_t>(argc));
        bool wrong = false;
        mrb_value result = mrb_nil_value();
        try {
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                auto read_all = std::make_tuple(
                    access::read<typename viewed<std::tuple_element_t<I, types>>::type>(call->time, mrb, given[I])...);
                if (!(std::get<I>(read_all).has_value() && ...)) {
                    wrong = true;
                    return;
                }
                if constexpr (std::is_void_v<typename parameters_of<F>::result>)
                    (*call->body)(*std::get<I>(read_all)...);
                else
                    result = mrb_nil_value(), (void)(*call->body)(*std::get<I>(read_all)...);
            }(std::make_index_sequence<count>{});
        } catch (...) {
            call->thrown = std::current_exception();
            mrb_raise(mrb, E_RUNTIME_ERROR, "a C++ block threw");
        }
        if (wrong) [[unlikely]]
            mrb_raise(mrb, E_TYPE_ERROR, "an argument of a C++ block has another type");
        return result;
    }

    template <class T> static constexpr bool callable_block = !object_type<T> && !scalar<T> &&
                                                             !std::same_as<T, std::nullptr_t> &&
                                                             !view_type<T>;

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
        time->open = false;
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
        return make<RString>([bytes](mrb_state *const mrb) {
            return mrb_str_new(mrb, bytes.data(), static_cast<mrb_int>(bytes.size()));
        });
    }
    RArray ary_new()
    {
        return make<RArray>([](mrb_state *const mrb) { return mrb_ary_new(mrb); });
    }
    RHash hash_new()
    {
        return make<RHash>([](mrb_state *const mrb) { return mrb_hash_new(mrb); });
    }
    value value_of(const mrb_int given)
    {
        mrb_state *const mrb = shared->mrb;
        const int arena = mrb_gc_arena_save(mrb);
        const mrb_value made = mrb_int_value(mrb, given);
        return value(borrowed(time, mrb, made, mrb_gc_arena_save(mrb) == arena ? -1 : arena));
    }
    value value_of(const mrb_float given)
    {
        mrb_state *const mrb = shared->mrb;
        const int arena = mrb_gc_arena_save(mrb);
        const mrb_value made = mrb_float_value(mrb, given);
        return value(borrowed(time, mrb, made, mrb_gc_arena_save(mrb) == arena ? -1 : arena));
    }
    value value_of(const bool given)
    {
        return value(borrowed(time, shared->mrb, mrb_bool_value(given)));
    }
    value value_of(std::nullptr_t)
    {
        return value(borrowed(time, shared->mrb, mrb_nil_value()));
    }
    RString::view str_new_view(const std::string_view bytes)
    {
        mrb_state *const mrb = shared->mrb;
        const int arena = mrb_gc_arena_save(mrb);
        return RString::view(
            borrowed(time, mrb, mrb_str_new(mrb, bytes.data(), static_cast<mrb_int>(bytes.size())), arena));
    }
    RArray::view ary_new_view()
    {
        mrb_state *const mrb = shared->mrb;
        const int arena = mrb_gc_arena_save(mrb);
        return RArray::view(borrowed(time, mrb, mrb_ary_new(mrb), arena));
    }
    RHash::view hash_new_view()
    {
        mrb_state *const mrb = shared->mrb;
        const int arena = mrb_gc_arena_save(mrb);
        return RHash::view(borrowed(time, mrb, mrb_hash_new(mrb), arena));
    }
    RClass define_class(const std::string &name)
    {
        return make<RClass>([&name](mrb_state *const mrb) {
            return mrb_obj_value(mrb_define_class(mrb, name.c_str(), mrb->object_class));
        });
    }

    template <mrb_sym Method, class Result = void, class Receiver, class... Args>
    auto funcall(const Receiver &receiver, const Args &...given)
    {
        mrb_state *const mrb = shared->mrb;
        const mrb_value self = argument(receiver);
        constexpr std::size_t count = sizeof...(Args);
        using last = std::tuple_element_t<count == 0 ? 0 : count - 1, std::tuple<Args..., void>>;
        constexpr bool with_block = count > 0 && callable_block<last>;
        constexpr std::size_t plain = with_block ? count - 1 : count;
        const int arena = mrb_gc_arena_save(mrb);
        std::experimental::scope_exit restore([mrb, arena] { mrb_gc_arena_restore(mrb, arena); });
        std::array<mrb_value, plain> argv{};
        mrb_value blk = mrb_nil_value();
        auto all = std::forward_as_tuple(given...);
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            ((argv[I] = argument(std::get<I>(all))), ...);
        }(std::make_index_sequence<plain>{});
        using body_type = std::conditional_t<with_block, std::remove_cvref_t<last>, int>;
        block<body_type> call{nullptr, nullptr, time};
        std::shared_ptr<lifetime> *life = nullptr;
        if constexpr (with_block) {
            call.body = &std::get<count - 1>(all);
            auto held = std::make_unique<std::shared_ptr<lifetime>>(std::make_shared<lifetime>());
            RData *const data = mrb_data_object_alloc(mrb, mrb->object_class, held.get(), &wrapper);
            life = held.release();
            const std::array<mrb_value, 2> env{mrb_cptr_value(mrb, &call), mrb_obj_value(data)};
            blk = mrb_obj_value(mrb_proc_new_cfunc_with_env(mrb, &block_call<body_type>, 2, env.data()));
        }
        const std::shared_ptr<lifetime> ended = life != nullptr ? *life : nullptr;
        const std::experimental::scope_exit end_block([&ended] {
            if (ended != nullptr)
                ended->alive = false;
        });
        ++time->epoch;
        const std::expected<mrb_value, mrb_value> answered = protect(mrb, [&](mrb_state *const m) {
            return mrb_funcall_with_block(m, self, Method, static_cast<mrb_int>(plain), argv.data(), blk);
        });
        ++time->epoch;
        if (call.thrown) [[unlikely]]
            std::rethrow_exception(call.thrown);
        if (!answered) [[unlikely]] {
            const mrb_value message = mrb_funcall_argv(mrb, answered.error(), mrb_intern_lit(mrb, "message"), 0,
                                                       nullptr);
            throw std::runtime_error(std::string(mrb_string_p(message) ? string_bytes(message) : "a Ruby raise"));
        }
        restore.release();
        mrb_gc_arena_restore(mrb, arena);
        mrb_gc_protect(mrb, *answered);
        if constexpr (!std::is_void_v<Result>)
            return access::read<Result>(time, mrb, *answered);
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
