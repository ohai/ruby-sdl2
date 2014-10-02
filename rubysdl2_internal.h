#include <ruby.h>

#define SDL_MAIN_HANDLED

int rubysdl2_handle_error(int code, const char* cfunc);

void rubysdl2_init_video(void);


#ifndef SDL2_EXTERN
#define SDL2_EXTERN extern
#endif

#define HANDLE_ERROR(c) (rubysdl2_handle_error((c), __func__))

SDL2_EXTERN VALUE mSDL2;
SDL2_EXTERN VALUE eSDL2Error;

