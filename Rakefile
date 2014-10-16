
task "pot" do
  sh "yard i18n -o doc/po/rubysdl2.pot main.c *.c"
end

task "init-po",["locale"] do |_, args|
  locale = args.locale || "ja"
  sh "rmsginit -i doc/po/rubysdl2.pot -o doc/po/#{locale}.po -l #{locale}"
end

task "merge-po",["locale"] do |_, args|
  locale = args.locale || "ja"
  sh "rmsgmerge -o doc/po/#{locale}.po doc/po/#{locale}.po doc/po/rubysdl2.pot"
end

task "gen-doc", ["locale"] do |_, args|
  locale = args.locale
  p locale
  if locale
    sh "yard doc -o doc/doc-#{locale} --locale #{locale} --po-dir doc/po main.c *.c"
  else
    sh "yard doc -o doc/doc-en main.c *.c"
  end
end

