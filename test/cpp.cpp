#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/gc.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/cpp.hpp>

#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>

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
        during = mrb_assoc_new(mrb, mrb_bool_value(mrb_frozen_p(object)), mrb_bool_value(mrb_frozen_p(object->c)));
    }
    return mrb_assoc_new(mrb, during, mrb_assoc_new(mrb, mrb_bool_value(mrb_frozen_p(object)), mrb_bool_value(mrb_frozen_p(object->c))));
}

/* Whether mruby::kept refuses a holder that has no instance variables. */
static mrb_value kept_on_integer_throws_q(mrb_state *mrb, mrb_value)
{
    try {
        mruby::kept field(mrb, mrb_fixnum_value(1), "children");
    } catch (const std::logic_error &) {
        return mrb_true_value();
    }
    return mrb_false_value();
}

template <class T>
concept made_with_new = requires { new T(static_cast<mrb_state *>(nullptr), mrb_nil_value()); } || requires { new T(mrb_nil_value()); };

/* Whether mruby::frozen and mruby::automatic refuse every way out of the
 * scope that made them: a copy, a move and new. */
static mrb_value scope_bound_q(mrb_state *, mrb_value)
{
    constexpr bool answered = !std::is_copy_constructible_v<mruby::frozen> && !std::is_move_constructible_v<mruby::frozen> &&
                              !made_with_new<mruby::frozen> && !std::is_copy_constructible_v<mruby::automatic> &&
                              !std::is_move_constructible_v<mruby::automatic> && !std::is_copy_assignable_v<mruby::automatic> &&
                              !made_with_new<mruby::automatic>;
    return mrb_bool_value(answered);
}

/* Whether a String that only a root holds survives a full collection in
 * a state that C++ owns. */
static mrb_value root_keeps_value_q(mrb_state *, mrb_value)
{
    const mruby::state owned;
    mrb_state *const mrb = owned.get();
    const int arena = mrb_gc_arena_save(mrb);
    const std::shared_ptr<const mrb_value> kept = mruby::root(owned, mruby::automatic(mrb_str_new_lit(mrb, "abc")));
    mrb_gc_arena_restore(mrb, arena);
    mrb_full_gc(mrb);
    return mrb_bool_value(RSTRING_LEN(*kept) == 3);
}

/* Whether a released root lets the collector free its object once the
 * next root has unregistered it. */
static mrb_value released_root_is_collected_q(mrb_state *, mrb_value)
{
    const mruby::state owned;
    mrb_state *const mrb = owned.get();
    const int arena = mrb_gc_arena_save(mrb);
    std::shared_ptr<const mrb_value> kept = mruby::root(owned, mruby::automatic(mrb_str_new_lit(mrb, "abc")));
    RBasic *const object = mrb_basic_ptr(*kept);
    kept.reset();
    const std::shared_ptr<const mrb_value> next = mruby::root(owned, mruby::automatic(mrb_nil_value()));
    mrb_gc_arena_restore(mrb, arena);
    mrb_full_gc(mrb);
    return mrb_bool_value(object->tt == MRB_TT_FREE);
}

/* Whether a root that outlives its state reads nil instead of freed memory,
 * and can be released afterwards. */
static mrb_value root_after_close_is_nil_q(mrb_state *, mrb_value)
{
    std::shared_ptr<const mrb_value> survivor;
    {
        const mruby::state owned;
        survivor = mruby::root(owned, mruby::automatic(mrb_str_new_lit(owned.get(), "abc")));
    }
    const bool nil = mrb_nil_p(*survivor);
    survivor.reset();
    return mrb_bool_value(nil);
}

/* Whether a raise in a state that C++ owns, where no Ruby frame is above,
 * comes back as a value, and a plain answer comes back as the answer. */
static mrb_value protect_returns_raise_q(mrb_state *, mrb_value)
{
    const mruby::state owned;
    mrb_state *const mrb = owned.get();
    const auto raised = mruby::protect(mrb, [](mrb_state *const mrb) -> mrb_value { mrb_raise(mrb, E_ARGUMENT_ERROR, "raised"); });
    const auto answered = mruby::protect(mrb, [](mrb_state *) { return mrb_fixnum_value(3); });
    return mrb_bool_value(!raised && mrb_obj_is_kind_of(mrb, raised.error(), E_ARGUMENT_ERROR) && answered && mrb_fixnum(*answered) == 3);
}

/* Applies one operation of mruby::kept to the holder, and answers what the
 * hidden instance variable holds afterwards. */
static mrb_value kept_after_m(mrb_state *mrb, mrb_value)
{
    mrb_value holder, value;
    mrb_sym how;
    mrb_get_args(mrb, "ono", &holder, &how, &value);
    mruby::kept field(mrb, holder, "children");
    const std::string_view operation = mrb_sym_name(mrb, how);
    if (operation == "assign") field.assign(value);
    else if (operation == "push_back") field.push_back(value);
    else if (operation == "erase") field.erase(value);
    else field.clear();
    return mrb_iv_get(mrb, holder, mruby::symbol<"__children__">(mrb));
}

struct counted {
    int n = 4;
};

/* Whether a std::weak_ptr to the C++ object of a mruby::data expires once
 * the collector frees the Ruby object, and whether the type check refuses a
 * value of another type. */
static mrb_value data_weak_expires_q(mrb_state *, mrb_value)
{
    const mruby::state owned;
    mrb_state *const mrb = owned.get();
    const int arena = mrb_gc_arena_save(mrb);
    RClass *const holder = mruby::data<counted>::define_class(mrb, "Counted", mrb->object_class);
    const mrb_value wrapped = mruby::data<counted>::wrap(mrb, holder, std::make_shared<counted>());
    const std::weak_ptr<counted> watched = mruby::data<counted>::get(mrb, wrapped);
    const bool read = !watched.expired() && watched.lock()->n == 4;
    const bool refused = mruby::data<counted>::get(mrb, mrb_str_new_lit(mrb, "abc")) == nullptr;
    mrb_gc_arena_restore(mrb, arena);
    mrb_full_gc(mrb);
    return mrb_bool_value(read && refused && watched.expired());
}

/* Makes a Counted object, the class that mruby::data::define_class makes. */
static mrb_value counted_new_m(mrb_state *mrb, mrb_value)
{
    RClass *const holder = mrb_class_defined(mrb, "Counted") ? mrb_class_get(mrb, "Counted") : mruby::data<counted>::define_class(mrb, "Counted", mrb->object_class);
    return mruby::data<counted>::wrap(mrb, holder, std::make_shared<counted>());
}

/* Whether mruby::data refuses to wrap into a class that define_class did
 * not make, where Ruby could copy the object without its C++ part. */
static mrb_value wrap_in_other_class_throws_q(mrb_state *mrb, mrb_value)
{
    try {
        mruby::data<counted>::wrap(mrb, mrb->object_class, std::make_shared<counted>());
    } catch (const std::logic_error &) {
        return mrb_true_value();
    }
    return mrb_false_value();
}

/* Whether the literal functions give back the bytes of the literal. */
static mrb_value literals_q(mrb_state *mrb, mrb_value)
{
    const mrb_value text = mruby::str_new_static<"abc">(mrb);
    return mrb_bool_value(mruby::symbol<"abc">(mrb) == mrb_intern_lit(mrb, "abc") && RSTRING_LEN(text) == 3);
}

extern "C" void mrb_mruby_cpp_gem_test(mrb_state *mrb)
{
    RClass *const test = mrb_define_module(mrb, "MrubyCppTest");
    mrb_define_module_function(mrb, test, "frozen_while_held?", frozen_while_held_q, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "append_while_frozen", append_while_frozen, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "frozen_after_inner?", frozen_after_inner_q, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "scope_bound?", scope_bound_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "root_keeps_value?", root_keeps_value_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "released_root_is_collected?", released_root_is_collected_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "root_after_close_is_nil?", root_after_close_is_nil_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "protect_returns_raise?", protect_returns_raise_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "kept_after", kept_after_m, MRB_ARGS_REQ(3));
    mrb_define_module_function(mrb, test, "singleton_frozen", singleton_frozen_m, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "kept_on_integer_throws?", kept_on_integer_throws_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "counted_new", counted_new_m, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "wrap_in_other_class_throws?", wrap_in_other_class_throws_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "data_weak_expires?", data_weak_expires_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "literals?", literals_q, MRB_ARGS_NONE());
}
