
SOURCE_FILES = "main.c *.c"

def yardoc(locale = nil)
  if locale
    sh "yard doc -o doc/doc-#{locale} --locale #{locale} --po-dir doc/po #{SOURCE_FILES}"
  else
    sh "yard doc -o doc/doc-en #{SOURCE_FILES}"
  end
end

task "pot" do
  sh "yard i18n -o doc/po/rubysdl2.pot #{SOURCE_FILES}"
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

