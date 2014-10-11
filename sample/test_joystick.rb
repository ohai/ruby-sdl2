require 'sdl2'

SDL2.init(SDL2::INIT_EVERYTHING)
SDL2::TTF.init

p SDL2::Joystick.num_connected_joysticks
exit if SDL2::Joystick.num_connected_joysticks == 0
$joystick = SDL2::Joystick.open(0)
p $joystick.name

window = SDL2::Window.create("testsprite",0, 0, 640, 480, 0)
                             
renderer = window.create_renderer(-1, 0)


loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::JoyButton
      p ev
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
