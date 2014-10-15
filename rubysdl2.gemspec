# -*- Ruby -*-
require_relative "lib/sdl2/version"

Gem::Specification.new do |spec|
  spec.name = "ruby-sdl2"
  spec.version = SDL2::VERSION
  spec.summary = "The simple ruby extension library for SDL 2.x"
  spec.description = <<-EOS
    Ruby/SDL2 is an extension library to use SDL 2.x
    (Simple DirectMedia Layer). SDL 2.x is quite refined
    from SDL 1.x and API is changed.
    This library enables you to control audio, keyboard,
    mouse, joystick, 3D hardware via OpenGL, and 2D video
    using opengl or Direct3D.
    Ruby/SDL is used by games and visual demos.
  EOS
  spec.license = "LGPL"
  spec.author = "Ohbayashi Ippei"
  spec.email = "ohai@kmc.gr.jp"
  spec.homepage = "https://github.com/ohai/ruby-sdl2"
  spec.files = `git ls-files`.split(/\n/)
  spec.test_files = []
  spec.extensions = ["extconf.rb"]
  spec.has_rdoc = false
end
