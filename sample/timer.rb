require 'sdl2'

SDL2.init(SDL2::INIT_TIMER)

p SDL2.get_ticks
p SDL2.get_performance_counter
p SDL2.get_performance_frequency
SDL2.delay 50
p SDL2.get_ticks
p SDL2.get_performance_counter
p SDL2.get_performance_frequency
