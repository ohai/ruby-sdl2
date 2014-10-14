#ifdef HAVE_SDL_FILESYSTEM_H
#include "rubysdl2_internal.h"
#include <SDL_filesystem.h>

static VALUE SDL2_s_base_path(VALUE self)
{
    char* path = SDL_GetBasePath();
    VALUE str;
    if (!path)
        SDL_ERROR();
    str = utf8str_new_cstr(path);
    SDL_Free(path);
    return str;
}

static VALUE SDL2_s_preference_path(VALUE self, VALUE org, VALUE app)
{
    char* path = SDL_GetPrefPath(StringValueCStr(org), StringValueCStr(app));
    VALUE str;
    if (!path)
        SDL_ERROR();
    str = utf8str_new_cstr(path);
    SDL_Free(path);
    return str;
}

void rubysdl2_init_filesystem(void)
{
    rb_define_module_function(mSDL2, "base_path", SDL2_s_base_path, 0);
    rb_define_module_function(mSDL2, "preference_path", SDL2_s_preference_path, 2);
}
#else
void rubysdl2_init_filesystem(void)
{
}
#endif
