require 'bundler/gem_tasks'

require 'rake/testtask'
require 'rake/extensiontask'
require 'rake/clean'
require 'yard'

Rake::ExtensionTask.new('sdl2_ext')

Rake::TestTask.new do |t|
  t.libs.push 'lib'
  t.pattern = "test/**/*_test.rb"
end

M4_SRC_FILES = FileList['ext/**/*.m4']
M4_TARGET_FILES = M4_SRC_FILES.ext('')
directory 'lib' => M4_TARGET_FILES
CLEAN.include M4_TARGET_FILES

M4_SRC_FILES.each do |m4_file|
  file m4_file.ext('') => m4_file do |t|
    sh "m4 #{t.source} > #{t.name}"
  end
end

namespace :yard do
  YARD_FILES = FileList.new
  YARD_LOCALES = %w(en jp)
  YARD_FILES.include('lib/**/*.rb')
            .include('ext/**/*.[ch]')
  POT_FILE = 'doc/po/sdl2.pot'

  file POT_FILE => YARD_FILES do |t|
    YARD::CLI::I18n.new.run '-o', t.name, *YARD_FILES
  end
  task :pot => POT_FILE

  YARD_LOCALES.each do |locale|
    YARD::Rake::YardocTask.new locale do |t|
      t.files = YARD_FILES
      t.options = %W(-m markdown --main README.md -o doc/#{locale})
      t.stats_options = %w(--list-undoc --compact)

      if locale != 'en'
        t.options.push '--po-dir', 'doc/po', '--locale', locale
      end
    end

    Rake::Task["yard:#{locale}"].enhance M4_TARGET_FILES
  end
end

