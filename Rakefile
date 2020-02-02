require 'rubygems'
require 'rubygems/package_task'
require 'rake/clean'
require_relative "lib/sdl2/version"

C_M4_FILES = Dir.glob("*.c.m4")
C_FROM_M4_FILES = C_M4_FILES.map{|path| path.gsub(/\.c\.m4\Z/, ".c") }
C_FILES = Dir.glob("*.c") | C_FROM_M4_FILES
RB_FILES = Dir.glob("lib/**/*.rb")
O_FILES = Rake::FileList[*C_FILES].pathmap('%n.o')

POT_SOURCES = RB_FILES + ["main.c"] + (C_FILES - ["main.c"])
YARD_SOURCES = "-m markdown --main README.md --files COPYING.txt #{POT_SOURCES.join(" ")}"

WATCH_TARGETS = (C_FILES - C_FROM_M4_FILES) + C_M4_FILES + ["README.md"]

CLEAN.include(O_FILES)
CLOBBER.include(*C_FROM_M4_FILES)

locale = ENV["YARD_LOCALE"]

def extconf_options
  return ENV["RUBYSDL2_EXTCONF_OPTS"] if ENV["RUBYSDL2_EXTCONF_OPTS"]
  return ENV["EXTCONF_OPTS"] if ENV["EXTCONF_OPTS"]
  
  begin
    return File.readlines("extconf-options.txt").map(&:chomp).join(" ")
  rescue Errno::ENOENT
    return ""
  end
end

def gem_spec
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
      Ruby/SDL is used for games and visual demos.
    EOS
    spec.license = "LGPL-3.0"
    spec.author = "Ippei Obayashi"
    spec.email = "ohai@kmc.gr.jp"
    spec.homepage = "https://github.com/ohai/ruby-sdl2"
    spec.files = `git ls-files`.split(/\n/) + C_FROM_M4_FILES
    spec.test_files = []
    spec.extensions = ["extconf.rb"]
  end
end

task "pot" => "doc/po/rubysdl2.pot"
file "doc/po/rubysdl2.pot" => POT_SOURCES do |t|
  sh "yard i18n -o #{t.name} #{POT_SOURCES.join(" ")}"
end

if locale
  PO_FILE = "doc/po/#{locale}.po"
  
  task "init-po" do
    sh "rmsginit -i doc/po/rubysdl2.pot -o #{PO_FILE} -l #{locale}"
  end

  task "merge-po" do
    sh "rmsgmerge -o #{PO_FILE} #{PO_FILE} doc/po/rubysdl2.pot"
  end

  task "doc" => [PO_FILE] do
    sh "yard doc -o doc/doc-#{locale} --locale #{locale} --po-dir doc/po #{YARD_SOURCES}"
  end
else
  task "doc" => POT_SOURCES do
    sh "yard doc -o doc/doc-en #{YARD_SOURCES}"
  end
end

task "doc-all" do
  raise "Not yet implemented"
end

desc "List undocumented classes/modules/methods/constants"
task "doc-stat-undocumented" => POT_SOURCES do
  sh "yard stats --list-undoc --compact #{YARD_SOURCES}"
end

rule ".c" => ".c.m4" do |t|
  sh "m4 #{t.prerequisites[0]} > #{t.name}"
end

file "Makefile" => "extconf.rb" do
  sh "ruby extconf.rb #{extconf_options()}"
end

task "build" => C_FILES + ["Makefile"] do
  sh "make"
end

Gem::PackageTask.new(gem_spec) do |pkg|
end

rule ".gem" => C_FILES

task "watch-doc" do
  loop do
    sh "inotifywait -e modify #{WATCH_TARGETS.join(" ")} && rake doc && notify-send -u low \"Ruby/SDL2 build doc OK\""
  end
end
