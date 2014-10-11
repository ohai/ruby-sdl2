require 'sdl2'

SDL2.init(SDL2::INIT_EVERYTHING)
SDL2::TTF.init

window = SDL2::Window.create("testsprite",
                             SDL2::Window::OP_CENTERED, SDL2::Window::OP_CENTERED,
                             640, 480, 0)
renderer = window.create_renderer(-1, 0)

font = SDL2::TTF.open("font.ttf", 24)

renderer.draw_color = [255,0,0]
renderer.fill_rect(SDL2::Rect.new(0,0,640,480))
renderer.copy(renderer.create_texture_from(font.render_solid("Foo", [255, 255, 255])),
              nil, SDL2::Rect.new(100, 100, 100, 30))

renderer.copy(renderer.create_texture_from(font.render_shaded("Foo", [255, 255, 255], [0,0,0])),
              nil, SDL2::Rect.new(100, 140, 100, 30))

renderer.copy(renderer.create_texture_from(font.render_blended("Foo", [255, 255, 255])),
              nil, SDL2::Rect.new(100, 180, 100, 30))

loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::KeyDown
      if ev.scancode == SDL2::Key::Scan::ESCAPE
        exit
      end
    when SDL2::Event::Quit
      exit
    end
  end

  renderer.present
  #GC.start
  sleep 0.1
end
