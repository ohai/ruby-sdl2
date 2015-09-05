#include "rubysdl2_internal.h"
#include <SDL_mouse.h>
#include <SDL_events.h>

static VALUE mMouse;
static VALUE cCursor;
static VALUE cState;

/*
 * Document-module: SDL2::Mouse
 *
 * This module has mouse input handling functions.
 * 
 */

/*
 * Document-class: SDL2::Mouse::Cursor
 *
 * This class represents mouse cursor shape, and
 * have some class-methods for a mouse cursor.
 */

/*
 * Document-class: SDL2::Mouse::State
 *
 * This class represents mouse stetes.
 *
 * You can get a mouse state with {SDL2::Mouse.state}.
 *
 * @!attribute [r] x
 *   the x coordinate of the mouse cursor.
 *
 *   For {SDL2::Mouse.state}, this attribute means the x coordinate
 *   relative to the window.
 *   For {SDL2::Mouse.relative_state}, this attribute means the x coordinate
 *   relative to the previous call of {SDL2::Mouse.relative_state}.
 *   
 *   @return [Integer]
 *
 * @!attribute [r] y
 *   the y coordinate of the mouse cursor
 *
 *   For {SDL2::Mouse.state}, this attribute means the y coordinate
 *   relative to the window.
 *   For {SDL2::Mouse.relative_state}, this attribute means the y coordinate
 *   relative to the previous call of {SDL2::Mouse.relative_state}.
 *   
 *   @return [Integer]
 *
 * @!attribute [r] button_bits
 *   button bits
 *   @return [Integer]
 *   @see #pressed?
 */

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
/*
 * Get the current mouse state in relation to the desktop.
 *
 * The return value contains the x and y coordinates of the cursor
 * relative to the desktop and the state of mouse buttons.
 *
 * @note This module function is available since SDL 2.0.4.
 * @return [SDL2::Mouse::State] current state
 */
static VALUE Mouse_s_global_state(VALUE self)
{
    return mouse_state(SDL_GetGlobalMouseState);
}
#endif

/*
 * Get the current state of the mouse.
 *
 * The return value contains the x and y coordinates of the cursor and
 * the state of mouse buttons.
 * 
 * @return [SDL2::Mouse::State] current state
 */
static VALUE Mouse_s_state(VALUE self)
{
    return mouse_state(SDL_GetMouseState);
}

/*
 * Return true if relative mouse mode is enabled.
 * 
 * @see .relative_mode=
 */
static VALUE Mouse_s_relative_mode_p(VALUE self)
{
    return INT2BOOL(SDL_GetRelativeMouseMode());
}

/*
 * @overload relative_mode=(bool)
 *   @param [Boolean] bool true if you want to enable relative mouse mode
 *
 * Set relative mouse mode.
 *
 * While the mouse is in relative mode, the cursor is hidden, and the
 * driver will try to report continuous motion in the current window.
 * Only relative motion events will be delivered, the mouse position
 * will not change.
 *
 * @note This function will flush any pending mouse motion.
 * 
 * @return [Boolean]
 * @see .relative_mode?
 */
static VALUE Mouse_s_set_relative_mode(VALUE self, VALUE enabled)
{
    HANDLE_ERROR(SDL_SetRelativeMouseMode(RTEST(enabled)));
    return enabled;
}

/*
 * Get the relative state of the mouse.
 *
 * The button state is same as {.state} and
 * x and y of the returned object are set to the
 * mouse deltas since the last call to this method.
 *
 * @return [SDL2::Mouse::State] 
 */
static VALUE Mouse_s_relative_state(VALUE self)
{
    return mouse_state(SDL_GetRelativeMouseState);
}

/*
 * Get the window which has mouse focus.
 *
 * @return [SDL2::Window] the window which has mouse focus
 * @return [nil] if no window governed by SDL has mouse focus
 */
static VALUE Mouse_s_focused_window(VALUE self)
{
    SDL_Window* window = SDL_GetMouseFocus();
    if (!window)
        return Qnil;
    else
        return find_window_by_id(SDL_GetWindowID(window));
}

/*
 * Show the mouse cursor.
 * @return [nil]
 */
static VALUE Cursor_s_show(VALUE self)
{
    HANDLE_ERROR(SDL_ShowCursor(SDL_ENABLE));
    return Qnil;
}

/*
 * Hide the mouse cursor.
 * @return [nil]
 */
static VALUE Cursor_s_hide(VALUE self)
{
    HANDLE_ERROR(SDL_ShowCursor(SDL_DISABLE));
    return Qnil;
}

/*
 * Return true if the mouse cursor is shown.
 */
static VALUE Cursor_s_shown_p(VALUE self)
{
    return INT2BOOL(HANDLE_ERROR(SDL_ShowCursor(SDL_QUERY)));
}

/*
 * @overload warp(window, x, y) 
 *   Move the mouse cursor to the given position within the window.
 *
 *   @param [SDL::Window] window the window to move the mouse cursor into
 *   @param [Integer] x the x coordinate within the window
 *   @param [Integer] y the y coordinate within the window
 *   @return [nil]
 */
static VALUE Cursor_s_warp(VALUE self, VALUE window, VALUE x, VALUE y)
{
    SDL_WarpMouseInWindow(Get_SDL_Window(window), NUM2INT(x), NUM2INT(y));
    return Qnil;
}

#if SDL_VERSION_ATLEAST(2,0,4)
/*
 * @overload warp_globally(x, y) 
 *   Move the mouse cursor to the given position within the desktop.
 *
 *   @note This class module function is available since SDL 2.0.4.
 *   @param [Integer] x the x coordinate within the desktop
 *   @param [Integer] y the y coordinate within the desktop
 *   @return [nil]
 */
static VALUE Cursor_s_warp_globally(VALUE self, VALUE x, VALUE y)
{
    SDL_WarpMouseGlobal(NUM2INT(x), NUM2INT(y));
    return Qnil;
}
#endif

/*
 * @overload pressed?(index)
 *   Return true a mouse button is pressed.
 *
 *   @param [Integer] index the index of a mouse button, start at index 0
 * 
 */
static VALUE State_pressed_p(VALUE self, VALUE index)
{
    int idx = NUM2INT(index);
    if (idx < 0 || idx >= 32)
        rb_raise(rb_eArgError, "button index out of range (%d for 1..32)", idx);
    return INT2BOOL(NUM2UINT(rb_iv_get(self, "@button_bits")) & SDL_BUTTON(idx));
}

/* @return [String] inspection string */
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
    rb_define_singleton_method(cCursor, "warp", Cursor_s_warp, 3);
#if SDL_VERSION_ATLEAST(2,0,4)
    rb_define_singleton_method(cCursor, "warp_globally", Cursor_s_warp_globally, 3);
#endif
    
    
    cState = rb_define_class_under(mMouse, "State", rb_cObject);

    rb_undef_method(rb_singleton_class(cState), "new");
    define_attr_readers(cState, "x", "y", "button_bits", NULL);
    rb_define_method(cState, "pressed?", State_pressed_p, 1);
    rb_define_method(cState, "inspect", State_inspect, 0);
}
