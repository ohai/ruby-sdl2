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
rect = SDL2::Rect.new(48, 128, 32, 32)
p rect.x
rect.x = 32
p rect.x

renderer.copy(texture, nil, rect)
renderer.present

loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::KeyDown
      if ev.scancode == 41
        exit
      else
        p ev.scancode
      end
    end
  end

  sleep 0.1
end

