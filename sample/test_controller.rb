require 'sdl2'

SDL2.init(SDL2::INIT_EVERYTHING)


p SDL2::GameController.axis_name_of(SDL2::GameController::AXIS_LEFTY)
p SDL2::GameController.axis_name_of(129) rescue p $!
p SDL2::GameController.axis_from_name("rightx")
p SDL2::GameController.button_name_of(SDL2::GameController::BUTTON_Y)
p SDL2::GameController.button_from_name("x")
p SDL2::GameController.button_from_name("rightx") rescue p $!

if SDL2::Joystick.num_connected_joysticks == 0
  puts "You need to connect at least one joystick"
  exit
end

joystick = SDL2::Joystick.open(0)
guid = joystick.GUID
joystick.close
SDL2::GameController.add_mapping([guid, "No1",
                                  "leftx:a0,lefty:a1",
                                  "a:b3,b:b2,x:b1,y:b0"
                                 ].join(","))


p SDL2::Joystick.game_controller?(0)
p SDL2::GameController.device_names
controller = SDL2::GameController.open(0)
p controller.name
p controller.attached?
p controller.mapping

$window = SDL2::Window.create("test_controller", 0,0,640,480,0)

loop do
  while event = SDL2::Event.poll
    case event
    when SDL2::Event::Quit
      exit
    when SDL2::Event::KeyDown
      exit if event.sym == SDL2::Key::ESCAPE
    when SDL2::Event::ControllerAxisMotion
      p event
      p controller.axis(SDL2::GameController::AXIS_LEFTX)
      p controller.axis(SDL2::GameController::AXIS_LEFTY)
    when SDL2::Event::ControllerButton
      p event
      p controller.button_pressed?(SDL2::GameController::BUTTON_A)
      p controller.button_pressed?(SDL2::GameController::BUTTON_B)
      p controller.button_pressed?(SDL2::GameController::BUTTON_X)
      p controller.button_pressed?(SDL2::GameController::BUTTON_Y)
    end
  end
end
