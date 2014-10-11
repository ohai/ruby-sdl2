require 'sdl2'

SDL2.init(SDL2::INIT_EVERYTHING)
SDL2::TTF.init


window = SDL2::Window.create("testsprite",
                             SDL2::Window::OP_CENTERED, SDL2::Window::OP_CENTERED,
                             640, 480, 0)
renderer = window.create_renderer(-1, 0)

font = SDL2::TTF.open("font.ttf", 40)

def draw_three_types(renderer, font, x, ybase)
  renderer.copy(renderer.create_texture_from(font.render_solid("Foo", [255, 255, 255])),
                nil, SDL2::Rect.new(x, ybase, 100, 30))

  renderer.copy(renderer.create_texture_from(font.render_shaded("Foo", [255, 255, 255], [0,0,0])),
                nil, SDL2::Rect.new(x, ybase+40, 100, 30))

  renderer.copy(renderer.create_texture_from(font.render_blended("Foo", [255, 255, 255])),
                nil, SDL2::Rect.new(x, ybase+80, 100, 30))
end


p font.style
p font.outline
p font.hinting
p font.kerning
p font.height
p font.ascent
p font.descent
p font.line_skip
p font.num_faces
p font.face_is_fixed_width?
p font.face_family_name
p font.face_style_name

renderer.draw_color = [255,0,0]
renderer.fill_rect(SDL2::Rect.new(0,0,640,480))

draw_three_types(renderer, font, 20, 50)

font.outline = 1
draw_three_types(renderer, font, 150, 50)

font.outline = 0
font.style = SDL2::TTF::STYLE_BOLD
draw_three_types(renderer, font, 280, 50)

font.style = 0
font.hinting = SDL2::TTF::HINTING_MONO
draw_three_types(renderer, font, 410, 50)

font.style = 0
font.hinting = SDL2::TTF::HINTING_NORMAL
font.kerning = false
draw_three_types(renderer, font, 540, 50)


loop do
  while ev = SDL2::Event.poll
    case ev
    when SDL2::Event::KeyDown
      if ev.scancode == SDL2::Key::Scan::ESCAPE
        exit
      end
    when SDL2::Event::Quit
      exit
    end
  end

  renderer.present
  #GC.start
  sleep 0.1
end
