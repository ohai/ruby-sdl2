begin
  require 'sdl2'
rescue LoadError
  $LOAD_PATH.unshift File.expand_path File.join(__dir__, '../lib')
  retry
end

SDL2.init(SDL2::INIT_EVERYTHING)
SDL2::TTF.init

p SDL2::Joystick.num_connected_joysticks
p SDL2::Joystick.devices

if SDL2::Joystick.num_connected_joysticks > 0
  $joystick = SDL2::Joystick.open(0)
  p $joystick.name
  p $joystick.num_axes
  p $joystick.num_hats
  p $joystick.num_buttons
  p $joystick.num_balls
  p $joystick.GUID
  p $joystick.attached?
  p $joystick.index
end

window = SDL2::Window.create("testsprite",0, 0, 640, 480, 0)
                             
renderer = window.create_renderer(-1, 0)


loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::JoyButton, SDL2::Event::JoyAxisMotion
      p ev
    when SDL2::Event::JoyDeviceAdded
      p ev
      $joystick = SDL2::Joystick.open(ev.which)
    when SDL2::Event::JoyDeviceRemoved
      p ev
      p $joystick.name
    when SDL2::Event::KeyDown
      if ev.scancode == SDL2::Key::Scan::ESCAPE
        exit
      end
    when SDL2::Event::Quit
      exit
    end
  end

  renderer.present
  sleep 0.1
end
