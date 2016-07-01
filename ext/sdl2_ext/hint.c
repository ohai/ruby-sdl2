#include "rubysdl2_internal.h"
#include <SDL_hints.h>

static VALUE sym_priority;

/*
 * Document-module: SDL2::Hints
 *
 * This module enables you to get and set configuration hints.
 *
 */

/*
 * Clear all hints set by {.[]=}.
 *
 * @return [nil]
 */
static VALUE Hints_s_clear(VALUE self)
{
    SDL_ClearHints();
    return Qnil;
}

/*
 * @overload [](hint)
 *   Get the value of a hint.
 *
 *   @param hint [String] the name of the hint to query
 *   
 * @return [String] the string value of a hint
 * @return [nil] if the hint isn't set
 */
static VALUE Hints_s_aref(VALUE self, VALUE name)
{
    const char* value = SDL_GetHint(StringValueCStr(name));
    if (value)
        return utf8str_new_cstr(value);
    else
        return Qnil;
}

/*
 * Set the value of a hint.
 * 
 * @overload []=(hint, value)
 *   Set a hint with normal priority.
 *
 *   @param hint [String] the name of the hint to query
 *   @param value [String] the value of the hint varaible
 *
 * @overload []=(hint, priority: , value)
 *   Set a hint with given priority.
 *
 *   @param hint [String] the name of the hint to query
 *   @param priority [Integer] the priority, one of the
 *     {DEFAULT}, {NORMAL}, or {OVERRIDE}.
 *   @param value [String] the value of the hint varaible
 *   
 * @return [Boolean] return true if the hint was set
 *
 * @example
 *   SDL2::Hints["SDL_HINT_XINPUT_ENABLED", priority: SDL2::Hints::OVERRIDE] = "0"
 *
 */
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
    rb_define_singleton_method(mHints, "get", Hints_s_aref, 1);
    rb_define_singleton_method(mHints, "[]=", Hints_s_aset, -1);

    /* low priority, used fro default values */
    rb_define_const(mHints, "DEFAULT", INT2NUM(SDL_HINT_DEFAULT));
    /* medium priority, overrided by an environment variable */
    rb_define_const(mHints, "NORMAL", INT2NUM(SDL_HINT_NORMAL));
    /* high priority, this priority overrides the value by environment variables */
    rb_define_const(mHints, "OVERRIDE", INT2NUM(SDL_HINT_OVERRIDE));

    sym_priority = ID2SYM(rb_intern("priority"));
}
