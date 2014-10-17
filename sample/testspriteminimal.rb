require 'sdl2'

SpriteMotion = Struct.new(:position, :velocity)
WINDOW_W = 640
WINDOW_H = 480
NUM_SPRITES = 100
MAX_SPEED = 1

def load_sprite(filename, renderer)
  bitmap = SDL2::Surface.load_bmp(filename)
  bitmap.color_key = bitmap.pixel(0, 0)
  sprite = renderer.create_texture_from(bitmap)
  bitmap.destroy

  sprite
end

def move_sprite(motions, renderer, sprite)
  # draw a gray background
  renderer.draw_color = [0xA0, 0xA0, 0xA0]
  renderer.clear
  
  sprite_w = sprite.w; sprite_h = sprite.h
  
  motions.each do |m|
    m.position.x += m.velocity.x
    if m.position.x < 0 || m.position.x >= WINDOW_W - sprite_w
      m.velocity.x *= -1
      m.position.x += m.velocity.x
    end

    m.position.y += m.velocity.y
    if m.position.y < 0 || m.position.y >= WINDOW_H - sprite_h
      m.velocity.y *= -1
      m.position.y += m.velocity.y
    end

    renderer.copy(sprite, nil, m.position)
  end

  renderer.present
end

def random_position(sprite_w, sprite_h)
  SDL2::Rect[rand(WINDOW_W - sprite_w), rand(WINDOW_H - sprite_h), sprite_w, sprite_h]
end

def random_velocity
  loop do
    x = rand(MAX_SPEED*2+1) - MAX_SPEED
    y = rand(MAX_SPEED*2+1) - MAX_SPEED
    return SDL2::Point.new(x, y) if x != 0 || y != 0
  end
end

window = SDL2::Window.create("testspritesimple", 0, 0, WINDOW_W, WINDOW_H, 0)
renderer = window.create_renderer(-1, 0)
sprite = load_sprite("icon.bmp", renderer)
sprite_w = sprite.w; sprite_h = sprite.h

motions = Array.new(NUM_SPRITES) do
  SpriteMotion.new(random_position(sprite_w, sprite_h), random_velocity)
end

loop do
  while event = SDL2::Event.poll
    case event
    when SDL2::Event::Quit, SDL2::Event::KeyDown
      exit
    end
  end

  move_sprite(motions, renderer, sprite)
  sleep 0.01
end
