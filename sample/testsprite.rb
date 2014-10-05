require "sdl2"

SDL2.init(SDL2::INIT_EVERYTHING)

window = SDL2::Window.create("testsprite",
                             SDL2::Window::OP_CENTERED, SDL2::Window::OP_CENTERED,
                             640, 480, 0)
renderer = window.create_renderer(-1, 0)
p window.debug_info
p renderer.debug_info
icon = SDL2::Surface.load_bmp("icon.bmp")
p icon.destroy?

texture = renderer.create_texture_from(icon)
icon.destroy

p icon.destroy?
rect = SDL2::Rect.new(48, 128, 16, 16)
p rect.x
rect.x = 32
p rect.x

renderer.copy_texture(texture, nil, rect)
sleep 1
