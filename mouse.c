#include "rubysdl2_internal.h"
#include <SDL_mouse.h>
#include <SDL_events.h>

static VALUE mMouse;
static VALUE cCursor;
static VALUE cState;

static VALUE State_new(int x, int y, Uint32 buttons)
{
    VALUE state = rb_obj_alloc(cState);
    rb_iv_set(state, "@x", INT2FIX(x));
    rb_iv_set(state, "@y", INT2FIX(y));
    rb_iv_set(state, "@button_bits", UINT2NUM(buttons));
    return state;
}

static VALUE mouse_state(Uint32 (*func)(int*,int*))
{
    int x, y;
    Uint32 state;
    state = func(&x, &y);
    return State_new(x, y, state);
}

#if SDL_VERSION_ATLEAST(2,0,4)
static VALUE Mouse_s_global_state(VALUE self)
{
    return mouse_state(SDL_GetGlobalMouseState);
}
#endif

static VALUE Mouse_s_state(VALUE self)
{
    return mouse_state(SDL_GetMouseState);
}

static VALUE Mouse_s_relative_mode_p(VALUE self)
{
    return INT2BOOL(SDL_GetRelativeMouseMode());
}

static VALUE Mouse_s_set_relative_mode(VALUE self, VALUE enabled)
{
    HANDLE_ERROR(SDL_SetRelativeMouseMode(RTEST(enabled)));
    return enabled;
}

static VALUE Mouse_s_relative_state(VALUE self)
{
    return mouse_state(SDL_GetRelativeMouseState);
}

static VALUE Mouse_s_focused_window(VALUE self)
{
    SDL_Window* window = SDL_GetMouseFocus();
    if (!window)
        return Qnil;
    else
        return find_window_by_id(SDL_GetWindowID(window));
}

static VALUE Cursor_s_show(VALUE self)
{
    HANDLE_ERROR(SDL_ShowCursor(SDL_ENABLE));
    return Qnil;
}

static VALUE Cursor_s_hide(VALUE self)
{
    HANDLE_ERROR(SDL_ShowCursor(SDL_DISABLE));
    return Qnil;
}

static VALUE Cursor_s_shown_p(VALUE self)
{
    return INT2BOOL(HANDLE_ERROR(SDL_ShowCursor(SDL_QUERY)));
}

static VALUE State_pressed_p(VALUE self, VALUE index)
{
    int idx = NUM2INT(index);
    if (idx < 0 || idx >= 32)
        rb_raise(rb_eArgError, "button index out of range (%d for 1..32)", idx);
    return INT2BOOL(NUM2UINT(rb_iv_get(self, "@button_bits")) & SDL_BUTTON(idx));
}

static VALUE State_inspect(VALUE self)
{
    int i;
    Uint32 buttons = NUM2UINT(rb_iv_get(self, "@button_bits"));
    VALUE pressed = rb_ary_new();
    VALUE string_for_pressed;
    for (i=1; i<=32; ++i) 
        if (buttons & SDL_BUTTON(i)) 
            rb_ary_push(pressed, rb_sprintf("%d", i));
    
    string_for_pressed = rb_ary_join(pressed, rb_str_new2(" "));
    return rb_sprintf("<%s:%p x=%d y=%d pressed=[%s]>",
                      rb_obj_classname(self), (void*)self,
                      NUM2INT(rb_iv_get(self, "@x")), NUM2INT(rb_iv_get(self, "@y")),
                      StringValueCStr(string_for_pressed));
}

void rubysdl2_init_mouse(void)
{
    mMouse = rb_define_module_under(mSDL2, "Mouse");
    
#if SDL_VERSION_ATLEAST(2,0,4)
    rb_define_module_function(mMouse, "global_state", Mouse_s_global_state, 0);
#endif
    rb_define_module_function(mMouse, "state", Mouse_s_state, 0);
    rb_define_module_function(mMouse, "relative_mode?", Mouse_s_relative_mode_p, 0);
    rb_define_module_function(mMouse, "relative_mode=", Mouse_s_set_relative_mode, 1);
    rb_define_module_function(mMouse, "relative_state", Mouse_s_relative_state, 0);
    rb_define_module_function(mMouse, "focused_window", Mouse_s_focused_window, 0);
    
    cCursor = rb_define_class_under(mMouse, "Cursor", rb_cObject);
    
    rb_define_singleton_method(cCursor, "show", Cursor_s_show, 0);
    rb_define_singleton_method(cCursor, "hide", Cursor_s_hide, 0);
    rb_define_singleton_method(cCursor, "shown?", Cursor_s_shown_p, 0);

    cState = rb_define_class_under(mMouse, "State", rb_cObject);

    rb_undef_method(rb_singleton_class(cState), "new");
    define_attr_readers(cState, "x", "y", "button_bits", NULL);
    rb_define_method(cState, "pressed?", State_pressed_p, 1);
    rb_define_method(cState, "inspect", State_inspect, 0);
}
