require 'mkmf'

pkg_config "sdl2"
pkg_config "SDL2_mixer"
pkg_config "SDL2_ttf"

have_header "SDL_image.h"
have_header "SDL_mixer.h"
have_header "SDL_ttf.h"
have_header("SDL_filesystem.h")

create_makefile('sdl2_ext')
