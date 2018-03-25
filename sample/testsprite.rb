require 'sdl2'
require 'optparse'

MAX_SPEED = 1

class Integer
  def bit?(mask)
    (self & mask) != 0
  end
end

class Cycle < Struct.new(:cycle_color, :cycle_alpha, :color, :alpha,
                         :color_direction, :alpha_direction)
  def update
    self.color += self.color_direction
    if self.color < 0
      self.color = 0
      self.color_direction *= -1
    end
    if color > 255
      self.color = 255
      self.color_direction *= -1
    end
    self.alpha += self.alpha_direction
    if self.alpha < 0
      self.alpha = 0
      self.alpha_direction *= -1
    end
    if self.alpha > 255
      self.alpha = 255
      self.alpha_direction *= -1
    end
  end
end
                   

class WindowData
  def initialize(sdl_window, renderer, cycle, blend_mode, use_color_key)
    @sdl_window = sdl_window
    @renderer = renderer
    @cycle = cycle
    @blend_mode = blend_mode
    @use_color_key = use_color_key
  end

  def setup(spritepath, num_sprites)
    load_sprite(spritepath)
    @sprite.blend_mode = @blend_mode
    @sprites = Array.new(num_sprites){
      Sprite.new(@sdl_window, @renderer, @sprite)
    }
  end
  
  def load_sprite(fname)
    bitmap = SDL2::Surface.load(fname)
    bitmap.color_key = bitmap.pixel(0, 0) if @use_color_key
    @sprite = @renderer.create_texture_from(bitmap)
    bitmap.destroy
  end

  def update
    @sprites.each(&:update)
  end

  def draw
    viewport = @renderer.viewport
    # Draw a gray background
    @renderer.draw_color = [0xA0, 0xA0, 0xA0]
    @renderer.clear

    # Points
    @renderer.draw_color = [0xff, 0, 0]
    @renderer.draw_point(0, 0)
    @renderer.draw_point(viewport.w - 1, 0)
    @renderer.draw_point(0 ,viewport.h - 1)
    @renderer.draw_point(viewport.w - 1, viewport.h - 1)

    # Fill rect
    @renderer.draw_color = [0xff, 0xff, 0xff]
    @renderer.fill_rect(SDL2::Rect[viewport.w/2 - 50, viewport.h/2 - 50, 100, 100])

    # Draw rect
    @renderer.draw_color = [0, 0, 0]
    @renderer.draw_rect(SDL2::Rect[viewport.w/2 - 50, viewport.h/2 - 50, 100, 100])
    
    # lines
    @renderer.draw_color = [0, 0xff, 0]
    @renderer.draw_line(1, 1, viewport.w - 2, viewport.h - 2)
    @renderer.draw_line(1, viewport.h - 2, viewport.w - 2, 1)

    @sprite.color_mod = [@cycle.color, 255, @cycle.color] if @cycle.cycle_color
    @sprite.alpha_mod = @cycle.alpha if @cycle.cycle_alpha
    
    @sprites.each(&:draw)
    @renderer.present
  end
  
  class Sprite
    def initialize(window, renderer, sprite, max_speed = MAX_SPEED)
      @renderer = renderer
      @sprite = sprite
      window_w, window_h = window.size
      @position = SDL2::Rect[rand(window_w - sprite.w),
                             rand(window_h - sprite.h),
                             sprite.w, sprite.h]
      @velocity = random_velocity(max_speed)
    end

    def random_velocity(max_speed)
      loop do
        dx = rand(2*max_speed + 1) - max_speed
        dy = rand(2*max_speed + 1) - max_speed
        return SDL2::Point[dx, dy] if dx != 0 || dy != 0
      end
    end

    def update
      viewport = @renderer.viewport
      @position.x += @velocity.x
      @position.y += @velocity.y
      unless (0 .. viewport.w - @sprite.w).cover?(@position.x)
        @velocity.x *= -1
        @position.x += @velocity.x
      end

      unless (0 .. viewport.h - @sprite.h).cover?(@position.y)
        @velocity.y *= -1
        @position.y += @velocity.y
      end
    end

    def draw
      @renderer.copy(@sprite, nil, @position)
    end
  end
end

class App
  def initialize
    @title = "testsprite"
    @window_flags = 0
    @icon_path = nil
    @num_window = 1
    @window_x = @window_y = SDL2::Window::POS_UNDEFINED
    @window_w = 640
    @window_h = 480
    @spritepath = "icon.bmp"
    @renderer_flags = 0
    @num_sprites = 100
    @blend_mode = SDL2::BlendMode::BLEND
    @cycle = Cycle.new(false, false, rand(255), rand(255), [1,-1].sample, [1,-1].sample)
    @use_color_key = true
  end

  def options
    opts = OptionParser.new("Usage: testsprite [options] [SPRITE]")
    opts.version = SDL2::VERSION
    opts.release = nil
    
    opts.on("-m", "--mode MODE", "fullscreen|fullscreen-desktop|window"){|mode|
      case mode
      when "fullscreen"
        raise "Only one window is available for fullscreen mode" if @num_window != 1
        @window_flags |= SDL2::Window::FULLSCREEN
      when "fullscreen-desktop"
        raise "Only one window is available for fullscreen-desktop mode" if @num_window != 1
        @window_flags |= SDL2::Window::FULLSCREEN_DESKTOP
      when "window"
        @window_flags &= ~(SDL2::Window::FULLSCREEN|SDL2::Window::FULLSCREEN_DESKTOP)
      end
    }
    
    opts.on("--windows N", "Number of windows", Integer){|n|
      if n != 1 and @window_flags.bit?(SDL2::Window::FULLSCREEN|SDL2::Window::FULLSCREEN_DESKTOP)
        raise "Only one window is available for fullscreen mode"
      end
      @num_window = n
    }

    opts.on("--geometry WxH", "size of the window/screen"){|s|
      raise "geomtry format error #{s}" unless /\A(\d+)x(\d+)\Z/ =~ s
      @window_w = Integer($1); @window_h = Integer($2)
    }

    opts.on("--center", "Window will be centered"){
      @window_x = @window_y = SDL2::Window::POS_CENTERED
    }

    opts.on("-t", "--title TITLE", "Window title"){|title| @title = title }

    opts.on("--icon ICON", "Window icon"){|path| @icon_path = path }

    opts.on("--position X,Y", "Position of the window", Array){|x,y|
      @window_x = Integer(x); @window_y = Integer(y)
    }

    opts.on("--noframe", "Window is borderless") { @window_flags |= SDL2::Window::BORDERLESS }

    opts.on("--resize", "Window is resizable") { @window_flags |= SDL2::Window::RESIZABLE }

    opts.on("--allow-highdip"){ @window_flags |= SDL2::Window::ALLOW_HIGHDPI }

    opts.on("--blend MODE", "none|blend|add|mod"){|blend_mode|
      @blend_mode = case blend_mode
                    when "none"
                      SDL2::BlendMode::NONE
                    when "blend"
                      SDL2::BlendMode::BLEND
                    when "add"
                      SDL2::BlendMode::ADD
                    when "mod"
                      SDL2::BlendMode::MOD
                    end
    }
    
    opts.on("--cycle-color"){ @cycle.cycle_color = true }

    opts.on("--cycle-alpha"){ @cycle.cycle_alpha = true }

    opts.on("--vsync", "Present is syncronized with the refresh rate") {
      @renderer_flags |= SDL2::Renderer::Flags::PRESENTVSYNC
    }

    opts.on("--use-color-key (yes|no)", TrueClass){|bool| @use_color_key = bool }
    opts
  end

  def run(argv)
    options.parse!(argv)
    @spritepath = argv.shift || @spritepath
    
    SDL2.init(SDL2::INIT_VIDEO)
    icon = load_icon
    @windows = @num_window.times.map do |n|
      window = SDL2::Window.create("#{@title} #{n}",
                                   @window_x, @window_y, @window_w, @window_h,
                                   @window_flags)
      renderer = window.create_renderer(-1, @renderer_flags)
      window.icon = icon if icon
      
      WindowData.new(window, renderer, @cycle, @blend_mode, @use_color_key)
    end

    @windows.each{|win| win.setup(@spritepath, @num_sprites) }

    loop do
      while event = SDL2::Event.poll
        handle_event(event)
      end
      @cycle.update
      @windows.each {|window| window.update; window.draw }
      sleep 0.01
    end
  end

  def load_icon
    return nil if @icon_path.nil?
    SDL2::Surface.load(@icon_path)
  end
  
  def handle_event(event)
    case event
    when SDL2::Event::Quit
      exit
    when SDL2::Event::KeyDown
      case event.sym
      when SDL2::Key::ESCAPE
        exit
      when SDL2::Key::RETURN
        if event.mod.bit?(SDL2::Key::Mod::ALT)
          if event.window.fullscreen_mode == 0
            event.window.fullscreen_mode = SDL2::Window::FULLSCREEN_DESKTOP
          else
            event.window.fullscreen_mode = 0
          end
        end
      when SDL2::Key::M
        if event.mod.bit?(SDL2::Key::Mod::CTRL|SDL2::Key::Mod::CAPS)
          window = event.window
          if window.flags & SDL2::Window::MAXIMIZED != 0
            window.restore
          else
            window.maximize
          end
        end
      when SDL2::Key::Z
        if event.mod.bit?(SDL2::Key::Mod::CTRL|SDL2::Key::Mod::CAPS)
          window = event.window
          window.minimize if window
        end
      end
    end
  end
end

App.new.run(ARGV)
