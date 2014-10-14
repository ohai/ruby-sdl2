#include "rubysdl2_internal.h"
#include <SDL_mouse.h>
#include <SDL_events.h>

static VALUE mMouse;
static VALUE cCursor;

static VALUE mouse_state(Uint32 (*func)(int*,int*))
{
    int x, y;
    Uint32 state;
    state = func(&x, &y);
    return rb_ary_new3(5, INT2FIX(x), INT2FIX(y),
                       INT2BOOL(state & SDL_BUTTON(1)),
                       INT2BOOL(state & SDL_BUTTON(2)),
                       INT2BOOL(state & SDL_BUTTON(3)));

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

    
    cCursor = rb_define_class_under(mMouse, "Cursor", rb_cObject);
    
    rb_define_singleton_method(cCursor, "show", Cursor_s_show, 0);
    rb_define_singleton_method(cCursor, "hide", Cursor_s_hide, 0);
    rb_define_singleton_method(cCursor, "shown?", Cursor_s_shown_p, 0);
    
}
