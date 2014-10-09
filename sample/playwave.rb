# This sample needs a wave file `sample.wav', please prepare.
require 'sdl2'

SDL2::init(SDL2::INIT_AUDIO)

SDL2::Mixer.open(22050, SDL2::Mixer::DEFAULT_FORMAT, 2, 512)
p SDL2::Mixer::Chunk.decoders
wave = SDL2::Mixer::Chunk.load(ARGV[0])

SDL2::Mixer.fadein_channel(0, wave, 0, 600)

while SDL2::Mixer.play?(0)
  sleep 1
end


