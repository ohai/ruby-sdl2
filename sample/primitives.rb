require "sdl2"

SDL2.init(SDL2::INIT_VIDEO|SDL2::INIT_EVENTS)

window = SDL2::Window.create("testsprite",
                             SDL2::Window::OP_CENTERED, SDL2::Window::OP_CENTERED,
                             640, 480, 0)
renderer = window.create_renderer(-1, 0)

renderer.draw_color(255, 0, 0, 255)
renderer.draw_line(0,0,640,480)
renderer.draw_color(255, 255, 0, 255)
renderer.draw_point(200, 200)
renderer.draw_color(0, 255, 255, 255)
renderer.draw_rect(SDL2::Rect.new(500, 20, 40, 60))
renderer.fill_rect(SDL2::Rect.new(20, 400, 60, 40))

renderer.present

loop do
  while ev = SDL2::Event.poll
    if SDL2::Event::KeyDown === ev && ev.scancode == SDL::Key::Scan::ESCAPE
      exit
    end
  end

  sleep 0.1
end
