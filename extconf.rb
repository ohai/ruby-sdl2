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

sdl2_config = with_config('sdl2-config', 'sdl2-config')
add_cflags(run_config_program(sdl2_config, "--cflags"))
add_libs(run_config_program(sdl2_config, "--libs"))

config("SDL2_image", "SDL_image.h", ["SDL2_image", "SDL_image"])
config("SDL2_mixer", "SDL_mixer.h", ["SDL2_mixer", "SDL_mixer"])
config("SDL2_ttf", "SDL_ttf.h", ["SDL2_ttf", "SDL_ttf"])

create_makefile('sdl2_ext')
