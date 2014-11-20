POT_SOURCES = "main.c *.c lib/**/*.rb"
SOURCE_FILES = "-m markdown --main README.md --files COPYING.txt #{POT_SOURCES}"

def yardoc(locale = nil)
  if locale
    sh "yard doc -o doc/doc-#{locale} --locale #{locale} --po-dir doc/po #{SOURCE_FILES}"
  else
    sh "yard doc -o doc/doc-en #{SOURCE_FILES}"
  end
end

def extconf_options
  return ENV["RUBYSDL2_EXTCONF_OPTS"] if ENV["RUBYSDL2_EXTCONF_OPTS"]
  return ENV["EXTCONF_OPTS"] if ENV["EXTCONF_OPTS"]
  
  begin
    return File.readlines("extconf-options.txt").map(&:chomp).join(" ")
  rescue Errno::ENOENT
    return ""
  end
end

task "pot" do
  sh "yard i18n -o doc/po/rubysdl2.pot #{POT_SOURCES}"
end

task "init-po",["locale"] do |_, args|
  locale = args.locale || "ja"
  sh "rmsginit -i doc/po/rubysdl2.pot -o doc/po/#{locale}.po -l #{locale}"
end

task "merge-po",["locale"] do |_, args|
  locale = args.locale || "ja"
  sh "rmsgmerge -o doc/po/#{locale}.po doc/po/#{locale}.po doc/po/rubysdl2.pot"
end

task "doc", ["locale"] do |_, args|
  locale = args.locale
  if locale == "all"
    yardoc
    yardoc("ja")
  elsif locale
    yardoc(locale)
  else
    yardoc
  end
end

rule ".c" => ".c.m4" do |t|
  sh "m4 #{t.prerequisites[0]} > #{t.name}"
end

file "key.c" => "key.c.m4" do
  sh "m4 key.c.m4 > key.c"
end

file "Makefile" => "extconf.rb" do
  sh "ruby extconf.rb #{extconf_options()}"
end

task "build" => ["key.c", "video.c", "Makefile"] do
  sh "make"
end

task "gem" => ["build"] do
  sh "gem build rubysdl2.gemspec"
end
