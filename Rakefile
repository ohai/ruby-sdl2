C_M4_FILES = Dir.glob("*.c.m4")
C_FROM_M4_FILES = C_M4_FILES.map{|path| path.gsub(/\.c\.m4\Z/, ".c") }
C_FILES = Dir.glob("*.c") | C_FROM_M4_FILES
RB_FILES = Dir.glob("lib/**/*.rb")

POT_SOURCES = RB_FILES + ["main.c"] + (C_FILES - ["main.c"])
YARD_SOURCES = "-m markdown --main README.md --files COPYING.txt #{POT_SOURCES.join(" ")}"

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

rule ".c" => ".c.m4" do |t|
  sh "m4 #{t.prerequisites[0]} > #{t.name}"
end

file "Makefile" => "extconf.rb" do
  sh "ruby extconf.rb #{extconf_options()}"
end

task "build" => C_FILES + ["Makefile"] do
  sh "make"
end

task "gem" => C_FILES do
  sh "gem build rubysdl2.gemspec"
end
