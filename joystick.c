#include "rubysdl2_internal.h"
#include <SDL_joystick.h>

static VALUE cJoystick;

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

static VALUE Joystick_s_open(VALUE self, VALUE device_index)
{
    SDL_Joystick* joystick = SDL_JoystickOpen(NUM2INT(device_index));
    if (!joystick)
        SDL_ERROR();
    return Joystick_new(joystick);
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

void rubysdl2_init_joystick(void)
{
    cJoystick = rb_define_class_under(mSDL2, "Joystick", rb_cObject);

    rb_define_singleton_method(cJoystick, "num_connected_joysticks",
                               Joystick_s_num_connected_joysticks, 0);
    rb_define_singleton_method(cJoystick, "open", Joystick_s_open, 1);
    rb_define_method(cJoystick, "destroy?", Joystick_destroy_p, 0);
    rb_define_alias(cJoystick, "close?", "destroy?");
    rb_define_method(cJoystick, "destroy", Joystick_destroy, 0);
    rb_define_alias(cJoystick, "close", "destroy");
    rb_define_method(cJoystick, "name", Joystick_name, 0);
}
