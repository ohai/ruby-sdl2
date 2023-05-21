module SDL2
  # @return [String] Version string of Ruby/SDL2
  VERSION = "0.3.6"
  # @return [Array<Integer>] Version of Ruby/SDL2, [major, minor, patch level] 
  VERSION_NUMBER = VERSION.split(".").map(&:to_i)
end
