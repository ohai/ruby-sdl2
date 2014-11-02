#include "rubysdl2_internal.h"
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>

static VALUE cJoystick;
static VALUE cDeviceInfo;

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

static VALUE Joystick_s_open(VALUE self, VALUE device_index)
{
    SDL_Joystick* joystick = SDL_JoystickOpen(NUM2INT(device_index));
    if (!joystick)
        SDL_ERROR();
    return Joystick_new(joystick);
}

static VALUE Joystick_s_game_controller_p(VALUE self, VALUE index)
{
    return INT2BOOL(SDL_IsGameController(NUM2INT(index)));
}

static VALUE Joystick_attached_p(VALUE self)
{
    Joystick* j = Get_Joystick(self);
    if (!j->joystick)
        return Qfalse;
    return INT2BOOL(SDL_JoystickGetAttached(j->joystick));
}

static VALUE Joystick_GUID(VALUE self)
{
    SDL_JoystickGUID guid;
    char buf[128];
    guid = SDL_JoystickGetGUID(Get_SDL_Joystick(self));
    SDL_JoystickGetGUIDString(guid, buf, sizeof(buf));
    return rb_usascii_str_new_cstr(buf);
}

static VALUE Joystick_index(VALUE self)
{
    return INT2NUM(HANDLE_ERROR(SDL_JoystickInstanceID(Get_SDL_Joystick(self))));
}

static VALUE Joystick_destroy(VALUE self)
{
    Joystick* j = Get_Joystick(self);
    if (j->joystick)
        SDL_JoystickClose(j->joystick);
    j->joystick = NULL;
    return Qnil;
}

static VALUE Joystick_name(VALUE self)
{
    return utf8str_new_cstr(SDL_JoystickName(Get_SDL_Joystick(self)));
}

static VALUE Joystick_num_axes(VALUE self)
{
    return INT2FIX(SDL_JoystickNumAxes(Get_SDL_Joystick(self)));
}

static VALUE Joystick_num_balls(VALUE self)
{
    return INT2FIX(SDL_JoystickNumBalls(Get_SDL_Joystick(self)));
}

static VALUE Joystick_num_buttons(VALUE self)
{
    return INT2FIX(SDL_JoystickNumButtons(Get_SDL_Joystick(self)));
}

static VALUE Joystick_num_hats(VALUE self)
{
    return INT2FIX(SDL_JoystickNumHats(Get_SDL_Joystick(self)));
}

static VALUE Joystick_axis(VALUE self, VALUE which)
{
    return INT2FIX(SDL_JoystickGetAxis(Get_SDL_Joystick(self), NUM2INT(which)));
}

static VALUE Joystick_ball(VALUE self, VALUE which)
{
    int dx, dy;
    HANDLE_ERROR(SDL_JoystickGetBall(Get_SDL_Joystick(self), NUM2INT(which), &dx, &dy));
    return rb_ary_new3(2, INT2NUM(dx), INT2NUM(dy));
}

static VALUE Joystick_button(VALUE self, VALUE which)
{
    return INT2BOOL(SDL_JoystickGetButton(Get_SDL_Joystick(self), NUM2INT(which)));
}

static VALUE Joystick_hat(VALUE self, VALUE which)
{
    return UINT2NUM(SDL_JoystickGetHat(Get_SDL_Joystick(self), NUM2INT(which)));
}
    
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
#define DEFINE_JOY_HAT_CONST(state) \
    rb_define_const(cJoystick, "HAT_" #state, INT2NUM(SDL_HAT_##state))
    DEFINE_JOY_HAT_CONST(CENTERED);
    DEFINE_JOY_HAT_CONST(UP);
    DEFINE_JOY_HAT_CONST(RIGHT);
    DEFINE_JOY_HAT_CONST(DOWN);
    DEFINE_JOY_HAT_CONST(LEFT);
    DEFINE_JOY_HAT_CONST(RIGHTUP);
    DEFINE_JOY_HAT_CONST(RIGHTDOWN);
    DEFINE_JOY_HAT_CONST(LEFTUP);
    DEFINE_JOY_HAT_CONST(LEFTDOWN);
    
    rb_define_attr(cDeviceInfo, "GUID", 1, 0);
    rb_define_attr(cDeviceInfo, "name", 1, 0);

    
}
