#include "rubysdl2_internal.h"
#include <SDL_hints.h>

static VALUE sym_priority;

static VALUE Hints_s_clear(VALUE self)
{
    SDL_ClearHints();
    return Qnil;
}

static VALUE Hints_s_aref(VALUE self, VALUE name)
{
    const char* value = SDL_GetHint(StringValueCStr(name));
    if (value)
        return utf8str_new_cstr(value);
    else
        return Qnil;
}

static VALUE Hints_s_aset(int argc, VALUE* argv, VALUE self)
{
    VALUE name, pri, value;
    rb_scan_args(argc, argv, "21", &name, &pri, &value);
    
    if (argc == 2) {
        value = pri;
        return INT2BOOL(SDL_SetHint(StringValueCStr(name), StringValueCStr(value)));
    } else {
        Check_Type(pri, T_HASH);
        return INT2BOOL(SDL_SetHintWithPriority(StringValueCStr(name),
                                                StringValueCStr(value),
                                                NUM2INT(rb_hash_aref(pri, sym_priority))));
    }
        
    return Qnil;
}

void rubysdl2_init_hints(void)
{
    VALUE mHints = rb_define_module_under(mSDL2, "Hints");
    
    rb_define_singleton_method(mHints, "clear", Hints_s_clear, 0);
    rb_define_singleton_method(mHints, "[]", Hints_s_aref, 1);
    rb_define_singleton_method(mHints, "get", Hints_s_aref, 1);
    rb_define_singleton_method(mHints, "[]=", Hints_s_aset, -1);
    rb_define_singleton_method(mHints, "set", Hints_s_aset, -1);
    
#define DEFINE_HINT_CONST(t) \
    rb_define_const(mHints, #t, INT2NUM(SDL_HINT_##t))
    DEFINE_HINT_CONST(DEFAULT);
    DEFINE_HINT_CONST(NORMAL);
    DEFINE_HINT_CONST(OVERRIDE);

    sym_priority = ID2SYM(rb_intern("priority"));
}
