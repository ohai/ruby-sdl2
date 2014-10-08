require 'sdl2'

SDL2.init(SDL2::INIT_VIDEO)

window = SDL2::Window.create("testsprite", 0, 0, 640, 480, 0)
renderer = window.create_renderer(-1, 0)
renderer.present

p SDL2.video_drivers
p SDL2::Renderer.drivers_info
p renderer.info
p renderer.clip_rect
p renderer.logical_size
p renderer.scale
p renderer.viewport
p renderer.support_render_target?
p renderer.output_size
