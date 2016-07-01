begin
  require 'sdl2'
rescue LoadError
  $LOAD_PATH.unshift File.expand_path File.join(__dir__, '../lib')
  retry
end


SDL2.init(SDL2::INIT_EVERYTHING)

window = SDL2::Window.create("testsprite",
                             SDL2::Window::POS_CENTERED, SDL2::Window::POS_CENTERED,
                             640, 480, 0)

renderer = window.create_renderer(-1,
                                  SDL2::Renderer::Flags::ACCELERATED|SDL2::Renderer::Flags::TARGETTEXTURE)
texture = renderer.load_texture("icon.bmp")

rect = SDL2::Rect.new(48, 128, 32, 32)
renderer.copy(texture, nil, rect)
renderer.copy_ex(texture, nil, SDL2::Rect[128, 256, 48, 48], 45, nil,
                 SDL2::Renderer::FLIP_NONE)
texture.blend_mode = SDL2::BlendMode::ADD
renderer.copy(texture, nil, SDL2::Rect[128, 294, 48, 48])

texture.blend_mode = SDL2::BlendMode::NONE
texture.color_mod = [128, 128, 255]
renderer.copy(texture, nil, SDL2::Rect[300, 420, 48, 48])

texture.blend_mode = SDL2::BlendMode::NONE
texture.color_mod = [255, 255, 255]

if renderer.support_render_target?
  empty_texture = renderer.create_texture(renderer.info.texture_formats.first,
                                          SDL2::Texture::ACCESS_TARGET, 128, 128)
  renderer.render_target = empty_texture
  renderer.draw_color = [255, 0, 255]
  renderer.draw_line(0, 0, 128, 128)
  renderer.reset_render_target
  renderer.copy(empty_texture, SDL2::Rect[0, 0, 128, 128], SDL2::Rect[420, 20, 128, 128])
end

loop do
  while ev = SDL2::Event.poll
    p ev
    case ev
    when SDL2::Event::KeyDown
      if ev.scancode == SDL2::Key::Scan::ESCAPE
        exit
      else
        p ev.scancode
      end
    end
  end

  renderer.present
  sleep 0.1
end

