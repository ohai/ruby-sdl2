#include <ruby.h>

#define SDL_MAIN_HANDLED

int rubysdl2_handle_error(int code, const char* cfunc);
int rubysdl2_is_active(void);

void rubysdl2_init_video(void);
void rubysdl2_init_event(void);
void rubysdl2_init_key(void);
void rubysdl2_init_timer(void);
void rubysdl2_init_image(void);

#ifndef SDL2_EXTERN
#define SDL2_EXTERN extern
#endif

/** macros */
#define HANDLE_ERROR(c) (rubysdl2_handle_error((c), __func__))
#define SDL_ERROR() (HANDLE_ERROR(-1))
#define INT2BOOL(x) ((x)?Qtrue:Qfalse)

/** classes and modules */
#define mSDL2 rubysdl2_mSDL2
#define eSDL2Error rubysdl2_eSDL2Error

SDL2_EXTERN VALUE mSDL2;
SDL2_EXTERN VALUE eSDL2Error;



