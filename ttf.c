#ifdef HAVE_SDL_TTF_H
#include "rubysdl2_internal.h"
#include <SDL_ttf.h>

VALUE cTTF;

#define TTF_ERROR() do { HANDLE_ERROR(SDL_SetError(TTF_GetError())); } while (0)
#define HANDLE_TTF_ERROR(code) \
    do { if ((code) < 0) { TTF_ERROR(); } } while (0)

VALUE TTF_s_init(VALUE self)
{
    HANDLE_TTF_ERROR(TTF_Init());
    return Qnil;
}


void rubysdl2_init_ttf(void)
{
    cTTF = rb_define_class_under(mSDL2, "TTF", rb_cObject);
    rb_define_singleton_method(cTTF, "init", TTF_s_init, 0);
}
#else
void rubysdl2_init_ttf(void)
{
}
#endif
