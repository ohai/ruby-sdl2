require 'sdl2'

SDL2.init(SDL2::INIT_EVERYTHING)
window = SDL2::Window.create("clipboard", 0, 0, 640, 480, 0)

# you can call clipboard functions after creating window (on X11)
p SDL2::Clipboard.has_text?
p SDL2::Clipboard.text
# text= is not reliable on X11 environment


