require "sdl2"

SDL2.init(SDL2::INIT_VIDEO|SDL2::INIT_EVENTS)

window = SDL2::Window.create("testsprite",
                             SDL2::Window::OP_CENTERED, SDL2::Window::OP_CENTERED,
                             640, 480, 0)
renderer = window.create_renderer(-1, 0)
surface = SDL2::Surface.load_bmp("icon.bmp")
10.times { renderer.create_texture_from(surface) }

window.destroy
