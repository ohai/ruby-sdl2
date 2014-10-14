require 'sdl2'

SDL2.init(SDL2::INIT_EVERYTHING)
window = SDL2::Window.create("testsprite",0, 0, 640, 480, 0)
renderer = window.create_renderer(0, SDL2::Renderer::ACCELERATED)
SDL2::TextInput.stop

loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::Quit
      exit
    when SDL2::Event::KeyDown
      case ev.sym
      when SDL2::Key::ESCAPE
        exit
      when SDL2::Key::S
        p SDL2::Mouse.state
      when SDL2::Key::R
        p SDL2::Mouse.relative_state        
      when SDL2::Key::SPACE
        SDL2::Mouse.relative_mode = ! SDL2::Mouse.relative_mode?
      when SDL2::Key::T
        if SDL2::Mouse::Cursor.shown?
          SDL2::Mouse::Cursor.hide
        else
          SDL2::Mouse::Cursor.show
        end
      end
    when SDL2::Event::MouseButtonDown
      p ev
    end
  end

  renderer.present
  sleep 0.01
end
