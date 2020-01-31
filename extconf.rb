require 'mkmf'

def add_cflags(str)
  $CFLAGS += " " + str
end

def add_libs(str)
  $LOCAL_LIBS += " " + str
end

def run_config_program(*args)
  IO.popen([*args], "r") do |io|
    io.gets.chomp
  end
end

def config(pkg_config, header, libnames)
  if system("pkg-config", pkg_config, "--exists")
    puts("Use pkg-config #{pkg_config}")
    add_cflags(run_config_program("pkg-config", pkg_config, "--cflags"))
    add_libs(run_config_program("pkg-config", pkg_config, "--libs"))
  else
    libnames.each{|libname| break if have_library(libname) }
  end
  
  have_header(header)
end

def sdl2config_with_command
  sdl2_config = with_config('sdl2-config', 'sdl2-config')
  add_cflags(run_config_program(sdl2_config, "--cflags"))
  add_libs(run_config_program(sdl2_config, "--libs"))
end

def sdl2config_on_mingw
  have_library("mingw32")
  have_library("SDL2")
  add_libs("-mwindows")
end

case RbConfig::CONFIG["arch"]
when /mingw/
  sdl2config_on_mingw
else
  sdl2config_with_command
end

config("SDL2_image", "SDL_image.h", ["SDL2_image", "SDL_image"])
config("SDL2_mixer", "SDL_mixer.h", ["SDL2_mixer", "SDL_mixer"])
config("SDL2_ttf", "SDL_ttf.h", ["SDL2_ttf", "SDL_ttf"])
have_header("SDL_filesystem.h")

have_const("MIX_INIT_MODPLUG", "SDL_mixer.h")
have_const("MIX_INIT_FLUIDSYNTH", "SDL_mixer.h")
have_const("MIX_INIT_MID", "SDL_mixer.h")
have_const("SDL_RENDERER_PRESENTVSYNC", "SDL_render.h")
have_const("SDL_WINDOW_ALLOW_HIGHDPI", "SDL_video.h")
have_const("SDL_WINDOW_MOUSE_CAPTURE", "SDL_video.h")

create_makefile('sdl2_ext')
