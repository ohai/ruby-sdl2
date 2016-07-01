require 'bundler/gem_tasks'

require 'rake/testtask'
require 'rake/extensiontask'
require 'yard'

Rake::ExtensionTask.new('sdl2_ext')

Rake::TestTask.new do |t|
  t.libs.push 'lib'
  t.pattern = "test/**/*_test.rb"
end

YARD::Rake::YardocTask.new do |t|
  t.files = ['lib/**/*.rb', 'ext/**/*.[ch]', 'COPYING.txt']
  t.options = %w(-m markdown --main README.md -o doc/doc-en)
  t.stats_options = %w(--list-undoc --compact)
end


