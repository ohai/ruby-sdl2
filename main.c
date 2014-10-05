#define SDL2_EXTERN
#include "rubysdl2_internal.h"
#include <SDL.h>
#include <stdio.h>

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

typedef enum {
  NOT_INITIALIZED, INITIALIZDED, FINALIZED
} sdl2_state;

sdl2_state state = NOT_INITIALIZED;

static void quit(VALUE unused)
{
  if (state != INITIALIZDED)
    return;
  
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

void Init_sdl2_ext(void)
{
  mSDL2 = rb_define_module("SDL2");

  rb_define_module_function(mSDL2, "init", SDL2_s_init, 1);
#define DEFINE_SDL_INIT_CONST(type) \
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
  
  eSDL2Error = rb_define_class_under(mSDL2, "Error", rb_eStandardError);
  rb_define_attr(eSDL2Error, "error_code", 1, 0);
  
  rubysdl2_init_video();
  rubysdl2_init_event();
  
  rb_set_end_proc(quit, 0);
  return;
}
