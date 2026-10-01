#include <mruby.h>
#include <mruby/string.h>
#include <mruby/gc.h>
#include <mruby/cpp.hpp>

#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>

/* Whether the value is frozen while a std::scoped_lock holds it. */
static mrb_value frozen_while_locked_q(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "o", &given);
    mruby::value held(given);
    const std::scoped_lock hold(held);
    return mrb_bool_value(!mrb_immediate_p(given) && mrb_frozen_p(mrb_basic_ptr(given)));
}

/* Appends to a String while a std::scoped_lock holds it. */
static mrb_value append_while_locked(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "S", &given);
    mruby::value held(given);
    const std::scoped_lock hold(held);
    mrb_str_cat_lit(mrb, given, "x");
    return given;
}

/* Whether the value is still frozen after the inner of two locks of one
 * mruby::value ended. */
static mrb_value frozen_after_inner_lock_q(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "o", &given);
    mruby::value held(given);
    const std::scoped_lock outer(held);
    {
        const std::scoped_lock inner(held);
    }
    return mrb_bool_value(mrb_frozen_p(mrb_basic_ptr(given)));
}

/* Whether a lock from a thread other than the one that made the
 * mruby::value throws std::logic_error. */
static mrb_value lock_from_other_thread_throws_q(mrb_state *mrb, mrb_value)
{
    mrb_value given;
    mrb_get_args(mrb, "o", &given);
    mruby::value held(given);
    bool thrown = false;
    std::thread other([&] {
        try {
            held.lock();
        } catch (const std::logic_error &) {
            thrown = true;
        }
    });
    other.join();
    return mrb_bool_value(thrown && !mrb_frozen_p(mrb_basic_ptr(given)));
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

template <class T>
concept made_with_new = requires { new T(mrb_nil_value()); };

/* Whether an automatic value refuses every way out of the call that made
 * it: a copy, a move and new. */
static mrb_value automatic_cannot_escape_q(mrb_state *, mrb_value)
{
    constexpr bool answered = !std::is_copy_constructible_v<mruby::automatic> && !std::is_move_constructible_v<mruby::automatic> &&
                              !std::is_copy_assignable_v<mruby::automatic> && !made_with_new<mruby::automatic>;
    return mrb_bool_value(answered);
}

extern "C" void mrb_mruby_cpp_gem_test(mrb_state *mrb)
{
    RClass *const test = mrb_define_module(mrb, "MrubyCppTest");
    mrb_define_module_function(mrb, test, "frozen_while_locked?", frozen_while_locked_q, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "append_while_locked", append_while_locked, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "frozen_after_inner_lock?", frozen_after_inner_lock_q, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "lock_from_other_thread_throws?", lock_from_other_thread_throws_q, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "root_keeps_value?", root_keeps_value_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "released_root_is_collected?", released_root_is_collected_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "root_after_close_is_nil?", root_after_close_is_nil_q, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, test, "automatic_cannot_escape?", automatic_cannot_escape_q, MRB_ARGS_NONE());
}
