/* -*- mode: C -*- */
#include "rubysdl2_internal.h"
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>

static VALUE cJoystick;
static VALUE cDeviceInfo;
static VALUE mHat;

typedef struct Joystick {
    SDL_Joystick* joystick;
} Joystick;

static void Joystick_free(Joystick* j)
{
    if (rubysdl2_is_active() && j->joystick)
        SDL_JoystickClose(j->joystick);
    free(j);
}

static VALUE Joystick_new(SDL_Joystick* joystick)
{
    Joystick* j = ALLOC(Joystick);
    j->joystick = joystick;
    return Data_Wrap_Struct(cJoystick, 0, Joystick_free, j);
}

DEFINE_WRAPPER(SDL_Joystick, Joystick, joystick, cJoystick, "SDL2::Joystick");

/*
 * Document-class: SDL2::Joystick
 *
 * This class represents a joystick connected to the machine.
 *
 * In order to use joystick subsystem, {SDL2.init} must have been called
 * with the SDL2::INIT_JOYSTICK flag.
 *
 * @!method destroy?
 *   Return true if the device is alread closed.
 *   @see #destroy
 */

/*
 * Get the number of connected joysticks.
 *
 * @return [Integer]
 */
static VALUE Joystick_s_num_connected_joysticks(VALUE self)
{
    return INT2FIX(HANDLE_ERROR(SDL_NumJoysticks()));
}

static VALUE GUID_to_String(SDL_JoystickGUID guid)
{
    char buf[128];
    SDL_JoystickGetGUIDString(guid, buf, sizeof(buf));
    return rb_usascii_str_new_cstr(buf);
}

/*
 * Get the information of connected joysticks
 *
 * @return [Array<SDL2::Joystick::DeviceInfo>] information of connected devices
 */
static VALUE Joystick_s_devices(VALUE self)
{
    int num_joysticks = SDL_NumJoysticks();
    int i;
    VALUE devices = rb_ary_new2(num_joysticks);
    for (i=0; i<num_joysticks; ++i) {
        VALUE device = rb_obj_alloc(cDeviceInfo);
        rb_iv_set(device, "@GUID", GUID_to_String(SDL_JoystickGetDeviceGUID(i)));
        rb_iv_set(device, "@name", utf8str_new_cstr(SDL_JoystickNameForIndex(i)));
        rb_ary_push(devices, device);
    }
    return devices;
}

/*
 * @overload open(device_index)
 *   Open a joystick for use.
 *
 *   @param [Integer] device_index device index
 *   @return [SDL2::Joystick] opended joystick object
 *   @raise [SDL2::Error] raised when device open is failed.
 *     for exmaple, device_index is out of range.
 */
static VALUE Joystick_s_open(VALUE self, VALUE device_index)
{
    SDL_Joystick* joystick = SDL_JoystickOpen(NUM2INT(device_index));
    if (!joystick)
        SDL_ERROR();
    return Joystick_new(joystick);
}

/*
 * @overload game_controller?(index)
 *   Return true if the joystick of given index supports the game controller
 *   interface.
 *
 *   @param [Integer] index the joystick device index
 *   @see SDL2::GameController
 * 
 */
static VALUE Joystick_s_game_controller_p(VALUE self, VALUE index)
{
    return INT2BOOL(SDL_IsGameController(NUM2INT(index)));
}

/*
 * Return true a joystick has been opened and currently connected.
 */
static VALUE Joystick_attached_p(VALUE self)
{
    Joystick* j = Get_Joystick(self);
    if (!j->joystick)
        return Qfalse;
    return INT2BOOL(SDL_JoystickGetAttached(j->joystick));
}

/*
 * Get the joystick GUID
 *
 * @return [String] GUID string
 */
static VALUE Joystick_GUID(VALUE self)
{
    SDL_JoystickGUID guid;
    char buf[128];
    guid = SDL_JoystickGetGUID(Get_SDL_Joystick(self));
    SDL_JoystickGetGUIDString(guid, buf, sizeof(buf));
    return rb_usascii_str_new_cstr(buf);
}

/*
 * Get the index of a joystick
 *
 * @return [Integer] index
 */
static VALUE Joystick_index(VALUE self)
{
    return INT2NUM(HANDLE_ERROR(SDL_JoystickInstanceID(Get_SDL_Joystick(self))));
}

/*
 * Close a joystick device.
 * 
 * @return [nil]
 * @see #destroy?
 */
static VALUE Joystick_destroy(VALUE self)
{
    Joystick* j = Get_Joystick(self);
    if (j->joystick)
        SDL_JoystickClose(j->joystick);
    j->joystick = NULL;
    return Qnil;
}

/*
 * Get the name of a joystick
 *
 * @return [String] name
 */
static VALUE Joystick_name(VALUE self)
{
    return utf8str_new_cstr(SDL_JoystickName(Get_SDL_Joystick(self)));
}

/*
 * Get the number of general axis controls on a joystick.
 * @return [Integer]
 * @see #axis
 */
static VALUE Joystick_num_axes(VALUE self)
{
    return INT2FIX(SDL_JoystickNumAxes(Get_SDL_Joystick(self)));
}

/*
 * Get the number of trackball on a joystick
 * @return [Integer]
 * @see #ball
 */
static VALUE Joystick_num_balls(VALUE self)
{
    return INT2FIX(SDL_JoystickNumBalls(Get_SDL_Joystick(self)));
}

/*
 * Get the number of button on a joystick
 * @return [Integer]
 * @see #button
 */
static VALUE Joystick_num_buttons(VALUE self)
{
    return INT2FIX(SDL_JoystickNumButtons(Get_SDL_Joystick(self)));
}

/*
 * Get the number of POV hats on a joystick
 * @return [Integer]
 * @see #hat
 */
static VALUE Joystick_num_hats(VALUE self)
{
    return INT2FIX(SDL_JoystickNumHats(Get_SDL_Joystick(self)));
}

/*
 * @overload axis(which) 
 *   Get the current state of an axis control on a joystick.
 *   
 *   @param [Integer] which an index of an axis, started at index 0
 *   @return [Integer] state value, ranging from -32768 to 32767.
 *   @see #num_axes
 */
static VALUE Joystick_axis(VALUE self, VALUE which)
{
    return INT2FIX(SDL_JoystickGetAxis(Get_SDL_Joystick(self), NUM2INT(which)));
}

/*
 * @overload ball(which) 
 *   Get the current state of a trackball on a joystick.
 *   
 *   @param [Integer] which an index of a trackball, started at index 0
 *   @return [[Integer,Integer]] dx and dy
 *   @see #num_balls
 */
static VALUE Joystick_ball(VALUE self, VALUE which)
{
    int dx, dy;
    HANDLE_ERROR(SDL_JoystickGetBall(Get_SDL_Joystick(self), NUM2INT(which), &dx, &dy));
    return rb_ary_new3(2, INT2NUM(dx), INT2NUM(dy));
}

/*
 * @overload button(which)
 *   Get the current state of a button on a joystick.
 *
 *   @param [Integer] which an index of a button, started at index 0
 *   @return [Boolean] true if the button is pressed
 *   @see #num_buttons
 */
static VALUE Joystick_button(VALUE self, VALUE which)
{
    return INT2BOOL(SDL_JoystickGetButton(Get_SDL_Joystick(self), NUM2INT(which)));
}

/*
 * @overload hat(which)
 *   Get the current state of a POV hat on a joystick.
 *
 *   @param [Integer] which an index of a hat, started at index 0
 *   @return [Integer] hat state
 *   @see #num_hats
 */
static VALUE Joystick_hat(VALUE self, VALUE which)
{
    return UINT2NUM(SDL_JoystickGetHat(Get_SDL_Joystick(self), NUM2INT(which)));
}

/*
 * Document-class: SDL2::Joystick::DeviceInfo
 *
 * This class represents joystick device information, its name and GUID.
 *
 * You can get the information with {SDL2::Joystick.devices}.
 */

/*
 * Document-module: SDL2::Joystick::Hat
 *
 * This module provides constants of joysticks's hat positions used by {SDL2::Joystick} class.
 * The position of the hat is represented by OR'd bits of {RIGHT}, {LEFT}, {UP}, and {DOWN}.
 * This means the center position ({CENTERED}) is represeted by 0 and
 * the left up position {LEFTUP} is represeted by ({LEFT}|{UP}).
 */

void rubysdl2_init_joystick(void)
{
    cJoystick = rb_define_class_under(mSDL2, "Joystick", rb_cObject);
    cDeviceInfo = rb_define_class_under(cJoystick, "DeviceInfo", rb_cObject);
    
    rb_define_singleton_method(cJoystick, "num_connected_joysticks",
                               Joystick_s_num_connected_joysticks, 0);
    rb_define_singleton_method(cJoystick, "devices", Joystick_s_devices, 0);
    rb_define_singleton_method(cJoystick, "open", Joystick_s_open, 1);
    rb_define_singleton_method(cJoystick, "game_controller?",
                               Joystick_s_game_controller_p, 1);
    rb_define_method(cJoystick, "destroy?", Joystick_destroy_p, 0);
    rb_define_alias(cJoystick, "close?", "destroy?");
    rb_define_method(cJoystick, "attached?", Joystick_attached_p, 0);
    rb_define_method(cJoystick, "GUID", Joystick_GUID, 0);
    rb_define_method(cJoystick, "index", Joystick_index, 0);
    rb_define_method(cJoystick, "destroy", Joystick_destroy, 0);
    rb_define_alias(cJoystick, "close", "destroy");
    rb_define_method(cJoystick, "name", Joystick_name, 0);
    rb_define_method(cJoystick, "num_axes", Joystick_num_axes, 0);
    rb_define_method(cJoystick, "num_balls", Joystick_num_balls, 0);
    rb_define_method(cJoystick, "num_buttons", Joystick_num_buttons, 0);
    rb_define_method(cJoystick, "num_hats", Joystick_num_hats, 0);
    rb_define_method(cJoystick, "axis", Joystick_axis, 1);
    rb_define_method(cJoystick, "ball", Joystick_ball, 1);
    rb_define_method(cJoystick, "button", Joystick_button, 1);
    rb_define_method(cJoystick, "hat", Joystick_hat, 1);

    mHat = rb_define_module_under(cJoystick, "Hat");
    
    /*  */
    /* Center position. Equal to 0. */
    rb_define_const(mHat, "CENTERED", INT2NUM(SDL_HAT_CENTERED));
    /* Up position. */
    rb_define_const(mHat, "UP", INT2NUM(SDL_HAT_UP));
    /* Right position. */
    rb_define_const(mHat, "RIGHT", INT2NUM(SDL_HAT_RIGHT));
    /* Down position. */
    rb_define_const(mHat, "DOWN", INT2NUM(SDL_HAT_DOWN));
    /* Left position. */
    rb_define_const(mHat, "LEFT", INT2NUM(SDL_HAT_LEFT));
    /* Right Up position. Equal to ({RIGHT} | {UP}) */
    rb_define_const(mHat, "RIGHTUP", INT2NUM(SDL_HAT_RIGHTUP));
    /* Right Down position. Equal to ({RIGHT} | {DOWN}) */
    rb_define_const(mHat, "RIGHTDOWN", INT2NUM(SDL_HAT_RIGHTDOWN));
    /* Left Up position. Equal to ({LEFT} | {UP}) */
    rb_define_const(mHat, "LEFTUP", INT2NUM(SDL_HAT_LEFTUP));
    /* Left Down position. Equal to ({LEFT} | {DOWN}) */
    rb_define_const(mHat, "LEFTDOWN", INT2NUM(SDL_HAT_LEFTDOWN));

    /* Device GUID
     * @return [String] */
    rb_define_attr(cDeviceInfo, "GUID", 1, 0);
    /* Device name
     * @return [String] */
    rb_define_attr(cDeviceInfo, "name", 1, 0);

    
}
