#ifdef HAVE_SDL_FILESYSTEM_H
#include "rubysdl2_internal.h"
#include <SDL_filesystem.h>

/*
 * Get the directory where the application was run from.
 *
 * In almost all cases, using this method, you will get the directory where ruby executable is in.
 * Probably what you hope is $0 or __FILE__.
 * 
 * @return [String] an absolute path in UTF-8 encoding to the application data directory
 * @raise [SDL2::Error] raised if your platform doesn't implement this functionality.
 * @note This method is available since SDL 2.0.1.
 */
static VALUE SDL2_s_base_path(VALUE self)
{
    char* path = SDL_GetBasePath();
    VALUE str;
    if (!path)
        SDL_ERROR();
    str = utf8str_new_cstr(path);
    SDL_free(path);
    return str;
}

/*
 * @overload preference_path(org, app)
 *   Get the "pref dir". You can use the directory to write personal files such as
 *   preferences and save games.
 *   
 *   The directory is unique per user and per application.
 *
 *   @param org [String] the name of your organization
 *   @param app [String] the name of your application
 *   @return [String] a UTF-8 string of the user directory in platform-depnedent notation.
 * 
 *   @raise [SDL2::Error] raised if your platform doesn't implement this functionality.
 *   @note This method is available since SDL 2.0.1.
 *
 *   @example
 *     SDL2.preference_path("foo", "bar") # => "/home/ohai/.local/share/foo/bar/" (on Linux)
 */
static VALUE SDL2_s_preference_path(VALUE self, VALUE org, VALUE app)
{
    char* path = SDL_GetPrefPath(StringValueCStr(org), StringValueCStr(app));
    VALUE str;
    if (!path)
        SDL_ERROR();
    str = utf8str_new_cstr(path);
    SDL_free(path);
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
