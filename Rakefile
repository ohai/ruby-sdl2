C_M4_FILES = Dir.glob("*.c.m4")
C_FROM_M4_FILES = C_M4_FILES.map{|path| path.gsub(/\.c\.m4\Z/, ".c") }
C_FILES = Dir.glob("*.c") | C_FROM_M4_FILES
RB_FILES = Dir.glob("lib/**/*.rb")

POT_FILES = ["main.c"] + (C_FILES - ["main.c"]) + RB_FILES
YARD_SOURCES = "-m markdown --main README.md --files COPYING.txt #{POT_FILES.join(" ")}"

def yardoc(locale = nil)
  if locale
    sh "yard doc -o doc/doc-#{locale} --locale #{locale} --po-dir doc/po #{YARD_SOURCES}"
  else
    sh "yard doc -o doc/doc-en #{YARD_SOURCES}"
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

task "pot" => "doc/po/rubysdl2.pot"
file "doc/po/rubysdl2.pot" => POT_FILES do |t|
  sh "yard i18n -o #{t.name} #{POT_FILES.join(" ")}"
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

file "Makefile" => "extconf.rb" do
  sh "ruby extconf.rb #{extconf_options()}"
end

task "build" => C_FILES + ["Makefile"] do
  sh "make"
end

task "gem" => C_FILES do
  sh "gem build rubysdl2.gemspec"
end
