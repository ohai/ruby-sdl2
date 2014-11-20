require "sdl2"

SDL2.init(SDL2::INIT_VIDEO|SDL2::INIT_EVENTS)

window = SDL2::Window.create("testsprite",
                             SDL2::Window::POS_CENTERED, SDL2::Window::POS_CENTERED,
                             640, 480, 0)

def f2(renderer)
  surface = SDL2::Surface.load_bmp("icon.bmp")
  10.times { renderer.create_texture_from(surface) }
  p renderer.debug_info
end

def f1(window)
  renderer = window.create_renderer(-1, 0)
  p window.debug_info
  p renderer.debug_info
  f2(renderer)
  GC.start
  p renderer.debug_info
end

f1(window)
GC.start
p window.debug_info


