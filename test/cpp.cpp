#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/gc.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/cpp.hpp>
#include <mruby/presym.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

/* Whether the value is frozen while a mruby::frozen exists. */
static mrb_value frozen_while_held_q(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "o", &given);
    const mruby::automatic argument(given);
    const mruby::frozen held(mrb, argument);
    return mrb_bool_value(!mrb_immediate_p(given) && mrb_frozen_p(mrb_basic_ptr(given)));
}

/* Appends to a String while a mruby::frozen holds it. */
static mrb_value append_while_frozen(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "S", &given);
    const mruby::automatic argument(given);
    const mruby::frozen held(mrb, argument);
    mrb_str_cat_lit(mrb, given, "x");
    return given;
}

/* Whether the value is still frozen after the inner of two mruby::frozen of
 * one object ended. */
static mrb_value frozen_after_inner_q(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "o", &given);
    const mruby::automatic argument(given);
    const mruby::frozen outer(mrb, argument);
    {
        const mruby::frozen inner(mrb, argument);
    }
    return mrb_bool_value(mrb_frozen_p(mrb_basic_ptr(given)));
}

/* Answers, for an object with a singleton class, whether the object and
 * its singleton class are frozen while a mruby::frozen exists and after
 * it ended. */
static mrb_value singleton_frozen_m(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "o", &given);
    RBasic *const object = mrb_basic_ptr(given);
    mrb_value during = mrb_nil_value();
    {
        const mruby::automatic argument(given);
        const mruby::frozen held(mrb, argument);
        during = mrb_assoc_new(mrb, mrb_bool_value(mrb_frozen_p(object)),
                               mrb_bool_value(mrb_frozen_p(object->c)));
    }
    return mrb_assoc_new(mrb, during,
                         mrb_assoc_new(mrb, mrb_bool_value(mrb_frozen_p(object)),
                                       mrb_bool_value(mrb_frozen_p(object->c))));
}

// Each requirement names a constructor the type has, so the concept is
// false only because operator new is deleted, and not because no
// constructor matches.
template <class T>
concept made_with_new = requires(mrb_state *mrb, const std::shared_ptr<const mrb_value> &held) {
    new T(mrb, held);
} || requires(mrb_value value) { new T(value); };

/* Whether mruby::frozen and mruby::automatic refuse every way out of the
 * scope that made them: a copy, a move and new. */
static mrb_value scope_bound_q(mrb_state *, mrb_value)
{
    constexpr bool answered =
        std::is_constructible_v<mruby::frozen, mrb_state *,
                                const std::shared_ptr<const mrb_value> &> &&
        std::is_constructible_v<mruby::automatic, mrb_value> &&
        !std::is_copy_constructible_v<mruby::frozen> &&
        !std::is_move_constructible_v<mruby::frozen> && !made_with_new<mruby::frozen> &&
        !std::is_copy_constructible_v<mruby::automatic> &&
        !std::is_move_constructible_v<mruby::automatic> &&
        !std::is_copy_assignable_v<mruby::automatic> && !made_with_new<mruby::automatic>;
    return mrb_bool_value(answered);
}

/* Whether a String that only a root holds survives a full collection in
 * a state that C++ owns. */
static mrb_value root_keeps_value_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    mrb_state *const mrb = owned.get();
    const int arena = mrb_gc_arena_save(mrb);
    const std::shared_ptr<const mrb_value> kept =
        owned.root(mruby::automatic(mrb_str_new_lit(mrb, "abc")));
    mrb_gc_arena_restore(mrb, arena);
    mrb_full_gc(mrb);
    return mrb_bool_value(RSTRING_LEN(*kept) == 3);
}

/* Whether a released root lets the collector free its object. The object is a data object whose
 * dfree sets a flag, because reading the object after the collection would read memory that mruby
 * may have given back. */
static mrb_value released_root_is_collected_q(mrb_state *, mrb_value)
{
    static constexpr mrb_data_type marks_freed{
        "marks_freed", [](mrb_state *, void *const p) { *static_cast<bool *>(p) = true; }};
    bool freed = false;
    mruby::state owned;
    mrb_state *const mrb = owned.get();
    const int arena = mrb_gc_arena_save(mrb);
    std::shared_ptr<const mrb_value> kept = owned.root(mruby::automatic(
        mrb_obj_value(mrb_data_object_alloc(mrb, mrb->object_class, &freed, &marks_freed))));
    kept.reset();
    mrb_gc_arena_restore(mrb, arena);
    mrb_full_gc(mrb);
    return mrb_bool_value(freed);
}

/* Whether a root that outlives its state reads undef instead of freed
 * memory, and can be released afterwards. */
static mrb_value root_after_close_is_undef_q(mrb_state *, mrb_value)
{
    std::shared_ptr<const mrb_value> survivor;
    {
        mruby::state owned;
        survivor = owned.root(mruby::automatic(mrb_str_new_lit(owned.get(), "abc")));
    }
    const bool undef = mrb_undef_p(*survivor);
    survivor.reset();
    return mrb_bool_value(undef);
}

struct holds_root {
    std::shared_ptr<const mrb_value> held;
    bool *released;
};

/* Whether a root that the dfree of a data object releases while mrb_close
 * frees the heap is released without a fault. */
static mrb_value root_released_in_close_q(mrb_state *, mrb_value)
{
    static constexpr mrb_data_type releases_root{"releases_root", [](mrb_state *, void *const p) {
                                                     holds_root *const holder =
                                                         static_cast<holds_root *>(p);
                                                     *holder->released = true;
                                                     delete holder;
                                                 }};
    bool released = false;
    {
        mruby::state owned;
        mrb_state *const mrb = owned.get();
        holds_root *const holder =
            new holds_root{owned.root(mruby::automatic(mrb_str_new_lit(mrb, "abc"))), &released};
        mrb_data_object_alloc(mrb, mrb->object_class, holder, &releases_root);
    }
    return mrb_bool_value(released);
}

/* Whether a raise in a state that C++ owns, where no Ruby frame is above,
 * comes back as a value, and a plain answer comes back as the answer. */
static mrb_value protect_returns_raise_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    mrb_state *const mrb = owned.get();
    const auto raised = mruby::protect(mrb, [](mrb_state *const mrb) -> mrb_value {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "raised");
        std::unreachable();
    });
    const auto answered = mruby::protect(mrb, [](mrb_state *) { return mrb_fixnum_value(3); });
    return mrb_bool_value(!raised && mrb_obj_is_kind_of(mrb, raised.error(), E_ARGUMENT_ERROR) &&
                          answered && mrb_fixnum(*answered) == 3);
}

/* Whether a C++ exception thrown inside mruby::protect leaves it as the same
 * C++ exception, and whether the state still runs Ruby afterwards. */
static mrb_value protect_rethrows_cxx_exception_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    mrb_state *const mrb = owned.get();
    bool caught = false;
    try {
        mruby::protect(mrb, [](mrb_state *) -> mrb_value { throw std::out_of_range("thrown"); });
    } catch (const std::out_of_range &) {
        caught = true;
    }
    const auto raised = mruby::protect(mrb, [](mrb_state *const mrb) -> mrb_value {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "raised");
        std::unreachable();
    });
    const auto answered = mruby::protect(mrb, [](mrb_state *const mrb) {
        return mrb_funcall(mrb, mrb_fixnum_value(1), "+", 1, mrb_fixnum_value(2));
    });
    return mrb_bool_value(caught && !raised && answered && mrb_fixnum(*answered) == 3);
}

/* Whether the literal functions give back the bytes of the literal. */
static mrb_value literals_q(mrb_state *mrb, mrb_value)
{
    const mrb_value text = mruby::str_new_static<"abc">(mrb);
    return mrb_bool_value(mruby::symbol<"abc">(mrb) == mrb_intern_lit(mrb, "abc") &&
                          RSTRING_LEN(text) == 3);
}


/* Makes one object of each type, runs a full GC, and answers whether each
 * one still reads what it was made with: the arena holds each wrapper, as
 * mrb_funcall holds its answer, and the wrapper holds the object. */
static mrb_value objects_survive_gc_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    const mruby::RString text = owned.str_new("abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz");
    mruby::RString copied = owned.str_new("abc");
    const mruby::RArray array = owned.ary_new();
    const mruby::RHash hash = owned.hash_new();
    const mruby::RClass klass = owned.define_class("MrubyCppMade");
    mrb_full_gc(owned.get());
    return mrb_bool_value(text.bytes().substr(0, 3) == "abc" && text.bytes().size() == 62 &&
                          copied.bytes() == "abc" && array.size() == 0 && hash.size() == 0 &&
                          klass.name() == "MrubyCppMade");
}

// The reason mruby-cpp exists: a method of one type does not compile on
// another, and no type converts to another, explicitly or implicitly.
template <class T>
concept has_bytes = requires(const T &t) { t.bytes(); };
template <class T>
concept has_size = requires(const T &t) { t.size(); };
template <class T>
concept has_name = requires(const T &t) { t.name(); };
template <class From, class To>
concept converts = std::is_convertible_v<From, To> || std::is_constructible_v<To, From>;

static mrb_value types_stay_apart_q(mrb_state *, mrb_value)
{
    using mruby::RArray, mruby::RClass, mruby::RHash, mruby::RString;
    const bool methods = has_bytes<RString> && !has_bytes<RHash> && !has_bytes<RArray> && !has_bytes<RClass> &&
                         has_size<RArray> && has_size<RHash> && !has_size<RString> && !has_size<RClass> &&
                         has_name<RClass> && !has_name<RString> && !has_name<RArray> && !has_name<RHash>;
    const bool apart = !converts<RString, RHash> && !converts<RHash, RString> && !converts<RString, RArray> &&
                       !converts<RArray, RString> && !converts<RArray, RHash> && !converts<RHash, RArray> &&
                       !converts<RClass, RString> && !converts<RString, RClass> && !converts<RClass, RHash> &&
                       !converts<RHash, RClass> && !converts<RClass, RArray> && !converts<RArray, RClass> &&
                       !converts<mrb_value, RString> && !converts<RString, mrb_value>;
    return mrb_bool_value(methods && apart);
}

/* Whether an mruby::RString throws when it is read after its state ended:
 * gem_final of mruby-cpp ends the lifetime of every wrapper before mruby
 * frees the heap. */
static mrb_value string_after_close_throws_q(mrb_state *, mrb_value)
{
    std::optional<mruby::RString> kept;
    {
        mruby::state owned;
        kept.emplace(owned.str_new("abc"));
        if (std::as_const(*kept).bytes() != "abc")
            return mrb_false_value();
    }
    try {
        (void)std::as_const(*kept).bytes();
    } catch (const std::logic_error &) {
        return mrb_true_value();
    }
    return mrb_false_value();
}


/* funcall with a presym: push returns the Array, size returns an Integer,
 * at<T> reads an element as the type it asks for and is empty for another
 * type or an index outside. */
static mrb_value funcall_and_at_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    const mruby::RArray array = owned.ary_new();
    const mruby::RString text = owned.str_new("abc");
    owned.funcall<MRB_SYM(push)>(array, text);
    owned.funcall<MRB_SYM(push)>(array, mrb_int{7});
    const std::optional<mrb_int> size = owned.funcall<MRB_SYM(size), mrb_int>(array);
    const std::optional<mruby::RString::view> first = array.at<mruby::RString>(0);
    const std::optional<mrb_int> second = array.at<mrb_int>(1);
    return mrb_bool_value(size == 2 && first && first->bytes() == "abc" && second == 7 &&
                          !array.at<mruby::RHash>(0) && !array.at<mruby::RString>(1) &&
                          !array.at<mruby::RString>(2));
}

/* A view read before a funcall throws after it, because Ruby ran in between.
 * An RString made from the view stays readable. */
static mrb_value view_ends_with_funcall_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    const mruby::RArray array = owned.ary_new();
    owned.funcall<MRB_SYM(push)>(array, owned.str_new("abc"));
    const mruby::RString::view seen = *array.at<mruby::RString>(0);
    const mruby::RString kept(seen);
    owned.funcall<MRB_SYM(size), mrb_int>(array);
    bool threw = false;
    try {
        (void)seen.bytes();
    } catch (const std::logic_error &) {
        threw = true;
    }
    return mrb_bool_value(threw && kept.bytes() == "abc");
}

/* get<T> finds a value by a String or an Integer key through
 * mrb_hash_fetch, and is empty for a missing key or another type. */
static mrb_value hash_get_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    const mruby::RHash hash = owned.hash_new();
    const mruby::RString key = owned.str_new("k");
    owned.funcall<MRB_OPSYM(aset)>(hash, key, owned.str_new("v"));
    owned.funcall<MRB_OPSYM(aset)>(hash, mrb_int{1}, mrb_int{2});
    const std::optional<mruby::RString::view> found = hash.get<mruby::RString>(key);
    return mrb_bool_value(found && found->bytes() == "v" && hash.get<mrb_int>(mrb_int{1}) == 2 &&
                          !hash.get<mrb_int>(mrb_int{3}) && !hash.get<mruby::RArray>(key));
}

/* A C++ lambda is the block of a call; each argument is checked against
 * its parameter type. */
static mrb_value lambda_block_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    const mruby::RArray array = owned.ary_new();
    owned.funcall<MRB_SYM(push)>(array, owned.str_new("ab"));
    owned.funcall<MRB_SYM(push)>(array, owned.str_new("cde"));
    std::size_t total = 0;
    owned.funcall<MRB_SYM(each)>(array, [&total](const mruby::RString::view text) {
        total += text.bytes().size();
    });
    bool wrong_type = false;
    try {
        owned.funcall<MRB_SYM(each)>(array, [](const mrb_int) {});
    } catch (const std::runtime_error &) {
        wrong_type = true;
    }
    return mrb_bool_value(total == 5 && wrong_type);
}

/* A C++ exception from the block reaches the caller of funcall as it is,
 * and a Ruby raise arrives as std::runtime_error with its message. */
static mrb_value errors_pass_q(mrb_state *, mrb_value)
{
    mruby::state owned;
    const mruby::RArray array = owned.ary_new();
    owned.funcall<MRB_SYM(push)>(array, mrb_int{1});
    bool cxx = false;
    try {
        owned.funcall<MRB_SYM(each)>(array, [](const mrb_int) { throw std::domain_error("from C++"); });
    } catch (const std::domain_error &) {
        cxx = true;
    }
    bool ruby = false;
    try {
        owned.funcall<MRB_SYM(raise)>(array, owned.str_new("raised in Ruby"));
    } catch (const std::runtime_error &e) {
        ruby = std::string_view(e.what()) == "raised in Ruby";
    }
    return mrb_bool_value(cxx && ruby);
}

extern "C" void mrb_mruby_cpp_gem_test(mrb_state *mrb)
{
    RClass *const test = mrb_define_module(mrb, "MrubyCppTest");
    mrb_define_module_function(mrb, test, "frozen_while_held?", frozen_while_held_q,
                               MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "append_while_frozen", append_while_frozen,
                               MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "frozen_after_inner?", frozen_after_inner_q,
                               MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "scope_bound?", scope_bound_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "root_keeps_value?", root_keeps_value_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "released_root_is_collected?",
                               released_root_is_collected_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "root_released_in_close?", root_released_in_close_q,
                               MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "root_after_close_is_undef?", root_after_close_is_undef_q,
                               MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "protect_returns_raise?", protect_returns_raise_q,
                               MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "protect_rethrows_cxx_exception?",
                               protect_rethrows_cxx_exception_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "singleton_frozen", singleton_frozen_m, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "literals?", literals_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "objects_survive_gc?", objects_survive_gc_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "types_stay_apart?", types_stay_apart_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "funcall_and_at?", funcall_and_at_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "view_ends_with_funcall?", view_ends_with_funcall_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "hash_get?", hash_get_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "lambda_block?", lambda_block_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "errors_pass?", errors_pass_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "string_after_close_throws?", string_after_close_throws_q,
                               MRB_ARGS_NONE());
}
