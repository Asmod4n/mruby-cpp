#include <mruby.h>
#include <mruby/string.h>
#include <mruby/cpp.hpp>

#include <mutex>
#include <stdexcept>
#include <thread>

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

extern "C" void mrb_mruby_cpp_gem_test(mrb_state *mrb)
{
    RClass *const test = mrb_define_module(mrb, "MrubyCppTest");
    mrb_define_module_function(mrb, test, "frozen_while_locked?", frozen_while_locked_q, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "append_while_locked", append_while_locked, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "frozen_after_inner_lock?", frozen_after_inner_lock_q, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, test, "lock_from_other_thread_throws?", lock_from_other_thread_throws_q, MRB_ARGS_REQ(1));
}
