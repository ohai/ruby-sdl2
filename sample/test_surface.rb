begin
  require 'sdl2'
rescue LoadError
  $LOAD_PATH.unshift File.expand_path File.join(__dir__, '../lib')
  retry
end

s = SDL2::Surface.load("icon.bmp")
p s.w
p s.h
p s.pitch
p s.bits_per_pixel
s2 = SDL2::Surface.from_string(s.pixels, 32, 32, 24)
