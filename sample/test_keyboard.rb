begin
  require 'sdl2'
rescue LoadError
  $LOAD_PATH.unshift File.expand_path File.join(__dir__, '../lib')
  retry
end

SDL2.init(SDL2::INIT_EVERYTHING)

window = SDL2::Window.create("testsprite", 0, 0, 640, 480, 0)
renderer = window.create_renderer(-1, 0)

puts "scancode of \"X\": #{SDL2::Key::Scan.from_name("X")}"
puts "scancode of SDL2::Key::X: #{SDL2::Key::Scan.from_keycode(SDL2::Key::X)}"
puts "keycode of \"X\": #{SDL2::Key.keycode_from_name("X")}"
puts "keycode of SDL2::Key::Scan::X: #{SDL2::Key.keycode_from_scancode(SDL2::Key::Scan::X)}"
SDL2::TextInput.rect = SDL2::Rect[20, 20, 200, 20]


def toggle_text_input
  if SDL2::TextInput.active?
    SDL2::TextInput.stop
  else
    SDL2::TextInput.start
  end
  p SDL2::TextInput.active?
end

loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::Quit
      exit
    when SDL2::Event::TextInput, SDL2::Event::TextEditing
      p ev
    when SDL2::Event::KeyDown
      puts "scancode: #{ev.scancode}(#{SDL2::Key::Scan.name_of(ev.scancode)})"
      puts "keycode: #{ev.sym}(#{SDL2::Key.name_of(ev.sym)})"
      puts "mod: #{ev.mod}"
      puts "mod(SDL2::Key::Mod.state): #{SDL2::Key::Mod.state}"
      if ev.sym == SDL2::Key::RETURN
        toggle_text_input
      end
    end
  end

  renderer.present
  sleep 0.01
end
