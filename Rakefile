require 'bundler/gem_tasks'

require 'rake/testtask'
require 'rake/extensiontask'

Rake::ExtensionTask.new('sdl2_ext')

Rake::TestTask.new do |t|
  t.libs.push 'lib'
  t.pattern = "test/**/*_test.rb"
end

#directory 'lib' => 'm4'

