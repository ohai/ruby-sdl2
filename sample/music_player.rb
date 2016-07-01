begin
  require 'sdl2'
rescue LoadError
  $LOAD_PATH.unshift File.expand_path File.join(__dir__, '../lib')
  retry
end

class TextureForText
  def initialize(renderer, font)
    @renderer = renderer
    @text_surfaces = {}
    @font = font
    @texture = nil
  end

  def add_text(text)
    @text_surfaces[text] = @font.render_blended(text, [0xff, 0xff, 0xff])
  end

  def creat_texture!
    @text_rects = Hash.new
    h = @text_surfaces.inject(0){|y, (text, surface)|
      @text_rects[text] = SDL2::Rect[0, y, surface.w, surface.h]
      y + surface.h
    }
    w = @text_rects.values.map(&:w).max

    tmp_surface = SDL2::Surface.new(w, h, 32)
    @text_surfaces.each do |text, surface|
      SDL2::Surface.blit(surface, nil, tmp_surface, @text_rects[text])
    end
    @texture = @renderer.create_texture_from(tmp_surface)
    tmp_surface.destroy
    
    @text_surfaces = nil
  end

  def box_for(text)
    @text_rects[text]
  end

  def render(text, dst)
    srcrect = @text_rects[text]
    dst = SDL2::Rect[dst.x, dst.y, srcrect.w, srcrect.h] if dst.is_a?(SDL2::Point)
    @renderer.copy(@texture, srcrect, dst)
  end
  
  attr_reader :texture
end

class MusicPlayer
  def run(argv)
    file = argv.shift
    
    SDL2.init(SDL2::INIT_EVERYTHING)
    SDL2::Mixer.init(SDL2::Mixer::INIT_FLAC|SDL2::Mixer::INIT_MOD|
                     SDL2::Mixer::INIT_MODPLUG|SDL2::Mixer::INIT_MP3|
                     SDL2::Mixer::INIT_OGG)
    SDL2::Mixer.open(44100, SDL2::Mixer::DEFAULT_FORMAT, 2, 512)
    SDL2::TTF.init

    @window = SDL2::Window.create("music player sample by Ruby/SDL2", 0, 0, 640, 480, 0)
    @renderer = @window.create_renderer(-1, 0)
    @music = SDL2::Mixer::Music.load(file)
    font = SDL2::TTF.open("font.ttf", 24)
    
    @texts = TextureForText.new(@renderer, font)
    @texts.add_text(file)
    @texts.add_text("playing")
    @texts.add_text("playing (fade in)")
    @texts.add_text("stopping")
    @texts.add_text("fade out")
    @texts.add_text("pause")
    @texts.creat_texture!
    
    loop do
      while event = SDL2::Event.poll
        handle_event(event)
      end

      @renderer.draw_color = [0, 0, 0]
      @renderer.clear
      
      @texts.render(file, SDL2::Point[20, 20])
      @texts.render(playing_state, SDL2::Point[20, 120])
      
      @renderer.present
      sleep 0.01
    end
  end

  def handle_event(event)
    case event
    when SDL2::Event::Quit
      exit
    when SDL2::Event::KeyDown
      keydown(event)
    end
  end

  def keydown(event)
    match_keydown("ESCAPE", event){ exit }
    match_keydown("p", event) { SDL2::Mixer::MusicChannel.play(@music, 1) }
    match_keydown("h", event) { SDL2::Mixer::MusicChannel.halt }
    match_keydown("r", event) { SDL2::Mixer::MusicChannel.rewind }
    match_keydown("o", event) { SDL2::Mixer::MusicChannel.fade_out(1000) }
    match_keydown("i", event) { SDL2::Mixer::MusicChannel.fade_in(@music, 1, 1000) }
    match_keydown("SPACE", event) {
      if SDL2::Mixer::MusicChannel.pause?
        SDL2::Mixer::MusicChannel.resume
      else
        SDL2::Mixer::MusicChannel.pause
      end
    }
  end

  def match_keydown(key, event)
    sym = SDL2::Key.keycode_from_name(key)
    yield if event.sym == sym
  end

  def playing_state
    return "stopping" if !SDL2::Mixer::MusicChannel.play?
    return "pause" if SDL2::Mixer::MusicChannel.pause?
    case SDL2::Mixer::MusicChannel.fading
    when SDL2::Mixer::NO_FADING
      return "playing"
    when SDL2::Mixer::FADING_IN
      return "playing (fade in)"
    when SDL2::Mixer::FADING_OUT
      return "fade out"
    end
  end
end


MusicPlayer.new.run(ARGV)
                             
