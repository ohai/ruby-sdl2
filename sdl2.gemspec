# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'sdl2/version'

Gem::Specification.new do |spec|
  spec.name = "sdl2"
  spec.version = SDL2::VERSION
  spec.summary = "The simple ruby extension library for SDL 2.x"
  spec.description = <<-EOS
    Ruby/SDL2 is an extension library to use SDL 2.x
    (Simple DirectMedia Layer). SDL 2.x is quite refined
    from SDL 1.x and API is changed.
    This library enables you to control audio, keyboard,
    mouse, joystick, 3D hardware via OpenGL, and 2D video
    using opengl or Direct3D.
    Ruby/SDL is used for games and visual demos.
  EOS
  spec.license = "LGPL-3.0"
  spec.author = "Ippei Obayashi"
  spec.email = "ohai@kmc.gr.jp"
  spec.homepage = "https://github.com/ohai/ruby-sdl2"
  spec.files = `git ls-files`.split(/\n/)
  spec.test_files = []
  spec.extensions = ["ext/sdl2_ext/extconf.rb"]
  spec.has_rdoc = false

  spec.add_development_dependency "rake", "~> 10.0"
  spec.add_development_dependency "yard", "~> 0.8"
  spec.add_development_dependency "rake-compiler", "~> 0.9"
end
