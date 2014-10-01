require 'mkmf'

sdl2_config = with_config('sdl2-config', 'sdl2-config')
$CFLAGS += " " + `#{sdl2_config} --cflags`.chomp
$LOCAL_LIBS += " " + `#{sdl2_config} --libs`.chomp

create_makefile('sdl2_ext')
