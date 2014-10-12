require 'sdl2'

SDL2.init(SDL2::INIT_EVERYTHING)

window = SDL2::Window.create("testsprite", 0, 0, 640, 480, 0)
renderer = window.create_renderer(-1, 0)

loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::Quit
      exit
    when SDL2::Event::KeyDown
      puts "scancode: #{ev.scancode}(#{SDL2::Key::Scan.name_of(ev.scancode)})"
      puts "keycode: #{ev.sym}(#{SDL2::Key.name_of(ev.sym)})"
      puts "mod: #{ev.mod}"
    end
  end
end
