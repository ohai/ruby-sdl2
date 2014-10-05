require "sdl2"

SDL2.init(SDL2::INIT_VIDEO|SDL2::INIT_EVENTS)

$window = SDL2::Window.create("testsprite", 0, 0, 128, 128, 0)

def f
  renderer = $window.create_renderer(-1, 0)
  surface = SDL2::Surface.load_bmp("icon.bmp")
  a = Array.new(10) { renderer.create_texture_from(surface) }
  p a.map{|tex| tex.debug_info }
  p renderer.debug_info
  a
end

a = f
GC.start
p a.map{|tex| tex.debug_info }
