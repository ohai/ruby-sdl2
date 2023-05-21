D = Steep::Diagnostic

# target :lib do
#   signature "sdl2.rbs"
#   library "rbs"

#   check "lib"
# end

target :sample_testspriteminimal do
  signature "sample/testspriteminimal.rbs"
  signature "sdl2.rbs"
  library "rbs"

  check "sample/testspriteminimal.rb"
  
  
  # configure_code_diagnostics(D::Ruby.strict)
  # configure_code_diagnostics(D::Ruby.lenient)
  configure_code_diagnostics do |hash|
    hash[D::Ruby::MethodDefinitionMissing] = :information
    hash[D::Ruby::UnknownConstant] = :information
  end
end
