#pragma once
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/value.h>
#include <mruby/variable.h>

#include <cstddef>
#include <exception>
#include <expected>
#include <memory>
#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

namespace mruby {

class frozen {
    RBasic *object = nullptr;
    bool thaws = false;

public:
    explicit frozen(const mrb_value given)
    {
        if (mrb_immediate_p(given)) return;
        object = mrb_basic_ptr(given);
        thaws = !mrb_frozen_p(object);
        object->frozen = 1;
    }
    frozen(const frozen &) = delete;
    frozen &operator=(const frozen &) = delete;
    static void *operator new(std::size_t) = delete;
    static void *operator new[](std::size_t) = delete;
    ~frozen()
    {
        if (thaws) object->frozen = 0;
    }
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
    return std::shared_ptr<const mrb_value>(cell.release(), [watched = std::weak_ptr<life>(shared)](const mrb_value *const p) {
        if (const std::shared_ptr<life> alive = watched.lock(); alive != nullptr) {
            const std::scoped_lock hold(alive->lock);
            if (alive->roots.erase(const_cast<mrb_value *>(p)) != 0) alive->released.push_back(*p);
        }
        delete p;
    });
}

template <class F>
std::expected<mrb_value, mrb_value> protect(mrb_state *const mrb, F &&body)
{
    mrb_bool error = false;
    const mrb_value answer = mrb_protect_error(
        mrb, [](mrb_state *const state, void *const given) -> mrb_value { return (*static_cast<std::remove_reference_t<F> *>(given))(state); }, &body, &error);
    if (error) [[unlikely]] return std::unexpected(answer);
    return answer;
}

template <mrb_value (*Function)(mrb_state *, mrb_value)>
mrb_value method(mrb_state *const mrb, const mrb_value self)
{
    mrb_value message = mrb_nil_value();
    try {
        return Function(mrb, self);
    } catch (const std::exception &thrown) {
        message = mrb_str_new_cstr(mrb, thrown.what());
    }
    mrb_exc_raise(mrb, mrb_exc_new_str(mrb, E_RUNTIME_ERROR, message));
}

class kept {
    mrb_state *mrb;
    mrb_value holder;
    mrb_sym name;

    mrb_value list() const
    {
        const mrb_value held = mrb_iv_get(mrb, holder, name);
        if (mrb_array_p(held)) return held;
        const mrb_value made = mrb_ary_new(mrb);
        mrb_iv_set(mrb, holder, name, made);
        return made;
    }

public:
    kept(mrb_state *const given, const mrb_value owner, const std::string_view field)
        : mrb(given), holder(owner), name(mrb_intern_cstr(given, ("__" + std::string(field) + "__").c_str()))
    {
    }
    void assign(const mrb_value value) { mrb_iv_set(mrb, holder, name, value); }
    void push_back(const mrb_value value) { mrb_ary_push(mrb, list(), value); }
    void erase(const mrb_value value)
    {
        const mrb_value held = list();
        const std::span<const mrb_value> values(RARRAY_PTR(held), static_cast<std::size_t>(RARRAY_LEN(held)));
        const auto found = std::ranges::find_if(values, [&](const mrb_value v) { return mrb_obj_eq(mrb, v, value); });
        if (found != values.end()) mrb_ary_splice(mrb, held, static_cast<mrb_int>(std::distance(values.begin(), found)), 1, mrb_undef_value());
    }
    void clear() { mrb_iv_remove(mrb, holder, name); }
};

template <class T>
struct data {
    inline static const mrb_data_type type{typeid(T).name(), [](mrb_state *, void *const p) { delete static_cast<std::shared_ptr<T> *>(p); }};
    static mrb_value wrap(mrb_state *const mrb, RClass *const klass, std::shared_ptr<T> object)
    {
        std::unique_ptr<std::shared_ptr<T>> held = std::make_unique<std::shared_ptr<T>>(std::move(object));
        RData *const made = mrb_data_object_alloc(mrb, klass, held.get(), &type);
        held.release();
        return mrb_obj_value(made);
    }
    static std::shared_ptr<T> get(mrb_state *const mrb, const mrb_value value)
    {
        const void *const p = mrb_data_check_get_ptr(mrb, value, &type);
        return p == nullptr ? nullptr : *static_cast<const std::shared_ptr<T> *>(p);
    }
};

template <std::size_t N>
struct literal {
    std::array<char, N> bytes{};
    consteval literal(const char (&given)[N]) { std::ranges::copy(given, bytes.begin()); }
    constexpr std::string_view view() const { return {bytes.data(), N - 1}; }
};

template <literal Name>
mrb_sym symbol(mrb_state *const mrb)
{
    return mrb_intern_static(mrb, Name.bytes.data(), Name.view().size());
}

template <literal Bytes>
mrb_value str_new_static(mrb_state *const mrb)
{
    return mrb_str_new_static(mrb, Bytes.bytes.data(), static_cast<mrb_int>(Bytes.view().size()));
}

}
