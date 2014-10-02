require "sdl2"

SDL2.init(SDL2::INIT_EVERYTHING)

window = SDL2::Window.create("testsprite", 0, 0, 640, 480, 0);

sleep 3
