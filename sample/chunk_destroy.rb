require 'sdl2'

SDL2::init(SDL2::INIT_AUDIO)

SDL2::Mixer.init(SDL2::Mixer::INIT_FLAC|SDL2::Mixer::INIT_MOD|
                 SDL2::Mixer::INIT_MP3|SDL2::Mixer::INIT_OGG)

SDL2::Mixer.open(22050, SDL2::Mixer::DEFAULT_FORMAT, 2, 512)

chunk = SDL2::Mixer::Chunk.load(ARGV[0])
p chunk; chunk.destroy; p chunk; p chunk.destroy?

mus = SDL2::Mixer::Music.load(ARGV[0])
p mus; mus.destroy; p mus; p mus.destroy?

GC.start
