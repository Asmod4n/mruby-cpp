#include <mruby.h>
#include <mruby/cpp.hpp>
#include <mruby/data.h>
#include <mruby/gc.h>

extern "C" void mrb_mruby_cpp_gem_init(mrb_state *)
{
}

extern "C" void mrb_mruby_cpp_gem_final(mrb_state *mrb)
{
    mrb_objspace_each_objects(
        mrb,
        [](mrb_state *const state, RBasic *const object, void *) -> int {
            if (object->tt != MRB_TT_CDATA)
                return MRB_EACH_OBJ_OK;
            RData *const data = reinterpret_cast<RData *>(object);
            if (data->type != &mruby::wrapper)
                return MRB_EACH_OBJ_OK;
            mruby::lifetime_end(state, data->data);
            data->data = nullptr;
            data->type = nullptr;
            return MRB_EACH_OBJ_OK;
        },
        nullptr);
}
