#define SDL2_EXTERN
#include "rubysdl2_internal.h"
#include <SDL.h>
#include <SDL_version.h>
#include <stdio.h>
#ifdef HAVE_SDL_IMAGE_H
#include <SDL_image.h>
#endif
#ifdef HAVE_SDL_MIXER_H
#include <SDL_mixer.h>
#endif
#ifdef HAVE_SDL_TTF_H
#include <SDL_ttf.h>
#endif
#include <stdarg.h>
#include <ruby/encoding.h>

int rubysdl2_handle_error(int code, const char* cfunc)
{
    VALUE err;
    char err_msg[1024];
  
    if (code >= 0)
        return code;

    snprintf(err_msg, sizeof(err_msg), "%s (cfunc=%s)", SDL_GetError(), cfunc);
    err = rb_exc_new2(eSDL2Error, err_msg);
    rb_iv_set(eSDL2Error, "@error_code", INT2NUM(code));
  
    rb_exc_raise(err);
}

void rubysdl2_define_attr_readers(VALUE klass, ...)
{
    va_list ap;

    va_start(ap, klass);
    for (;;) {
        const char* field_name = va_arg(ap, const char*);
        if (field_name == NULL)
            break;
        rb_define_attr(klass, field_name, 1, 0);
    }
    va_end(ap);
}

VALUE utf8str_new_cstr(const char* str)
{
    return rb_enc_str_new(str, strlen(str), rb_utf8_encoding());
}

VALUE SDL_version_to_String(const SDL_version* ver)
{
    return rb_sprintf("%d.%d.%d", ver->major, ver->minor, ver->patch);
}

VALUE SDL_version_to_Array(const SDL_version* ver)
{
    return rb_ary_new3(3, INT2FIX(ver->major), INT2FIX(ver->minor), INT2FIX(ver->patch));
}

const char* INT2BOOLCSTR(int n)
{
    return n ? "true" : "false";
}

typedef enum {
    NOT_INITIALIZED, INITIALIZDED, FINALIZED
} sdl2_state;

static sdl2_state state = NOT_INITIALIZED;

static void quit(VALUE unused)
{
    if (state != INITIALIZDED)
        return;
    
#ifdef HAVE_SDL_IMAGE_H
    IMG_Quit();
#endif
#ifdef HAVE_SDL_MIXER_H
    Mix_Quit();
#endif
#ifdef HAVE_SDL_TTF_H
    TTF_Quit();
#endif
    SDL_Quit();
    state = FINALIZED;
}

static VALUE SDL2_s_init(VALUE self, VALUE flags)
{
    SDL_SetMainReady();
    HANDLE_ERROR(SDL_Init(NUM2UINT(flags)));
    state = INITIALIZDED;
    return Qnil;
}

int rubysdl2_is_active(void)
{
    return state == INITIALIZDED;
}

static VALUE libsdl_version(void)
{
    SDL_version version;
    SDL_GetVersion(&version);
    return SDL_version_to_String(&version);
}

static VALUE libsdl_version_number(void)
{
    SDL_version version;
    SDL_GetVersion(&version);
    return SDL_version_to_Array(&version);
}

static VALUE libsdl_revision(void)
{
    return rb_usascii_str_new_cstr(SDL_GetRevision());
}

static VALUE libsdl_revision_number(void)
{
    return INT2NUM(SDL_GetRevisionNumber());
}

void Init_sdl2_ext(void)
{
    mSDL2 = rb_define_module("SDL2");

    rb_define_module_function(mSDL2, "init", SDL2_s_init, 1);
#define DEFINE_SDL_INIT_CONST(type)                                     \
    rb_define_const(mSDL2, "INIT_" #type, UINT2NUM(SDL_INIT_##type))
  
    DEFINE_SDL_INIT_CONST(TIMER);
    DEFINE_SDL_INIT_CONST(AUDIO);
    DEFINE_SDL_INIT_CONST(VIDEO);
    DEFINE_SDL_INIT_CONST(JOYSTICK);
    DEFINE_SDL_INIT_CONST(HAPTIC);
    DEFINE_SDL_INIT_CONST(GAMECONTROLLER);
    DEFINE_SDL_INIT_CONST(EVENTS);
    DEFINE_SDL_INIT_CONST(EVERYTHING);
    DEFINE_SDL_INIT_CONST(NOPARACHUTE);

    rb_define_const(mSDL2, "LIBSDL_VERSION", libsdl_version());
    rb_define_const(mSDL2, "LIBSDL_VERSION_NUMBER", libsdl_version_number());
    rb_define_const(mSDL2, "LIBSDL_REVISION", libsdl_revision());
    rb_define_const(mSDL2, "LIBSDL_REVISION_NUMBER", libsdl_revision_number());

#ifdef HAVE_SDL_IMAGE_H
    {
        const SDL_version* version = IMG_Linked_Version();
        rb_define_const(mSDL2, "LIBSDL_IMAGE_VERSION", SDL_version_to_String(version));
        rb_define_const(mSDL2, "LIBSDL_IMAGE_VERSION_NUMBER", SDL_version_to_Array(version));
    }
#endif
#ifdef HAVE_SDL_TTF_H
    {
        const SDL_version* version = TTF_Linked_Version();
        rb_define_const(mSDL2, "LIBSDL_TTF_VERSION", SDL_version_to_String(version));
        rb_define_const(mSDL2, "LIBSDL_TTF_VERSION_NUMBER", SDL_version_to_Array(version));
    }
#endif
#ifdef HAVE_SDL_MIXER_H
    {
        const SDL_version* version = Mix_Linked_Version();
        rb_define_const(mSDL2, "LIBSDL_MIXER_VERSION", SDL_version_to_String(version));
        rb_define_const(mSDL2, "LIBSDL_MIXER_VERSION_NUMBER", SDL_version_to_Array(version));
    }
#endif
    
    eSDL2Error = rb_define_class_under(mSDL2, "Error", rb_eStandardError);
    rb_define_attr(eSDL2Error, "error_code", 1, 0);
  
    rubysdl2_init_video();
    rubysdl2_init_event();
    rubysdl2_init_key();
    rubysdl2_init_joystick();
    rubysdl2_init_timer();
    rubysdl2_init_image();
    rubysdl2_init_mixer();
    rubysdl2_init_ttf();
    
    rb_set_end_proc(quit, 0);
    return;
}
