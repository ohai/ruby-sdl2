require 'sdl2'

SDL2.init(SDL2::INIT_VIDEO)

window = SDL2::Window.create("testsprite", 0, 0, 640, 480, 0)
renderer = window.create_renderer(-1, 0)
p SDL2.video_drivers
p SDL2::Renderer.drivers_info
p renderer.info

