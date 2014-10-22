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
SDL2::GameController.add_mapping([guid,"No1",
                                  "leftx:a0,lefty:a1",
                                  "a:b0,b:b1,x:b2,y:b3"
                                 ].join(","))


p SDL2::Joystick.game_controller?(0)
p SDL2::GameController.device_names
controller = SDL2::GameController.open(0)
p controller.name
p controller.attached?
p controller.mapping
