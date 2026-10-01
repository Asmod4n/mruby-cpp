#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/gc.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/cpp.hpp>

#include <memory>
#include <stdexcept>
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
}
