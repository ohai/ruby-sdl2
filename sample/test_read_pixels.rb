require "sdl2"

SDL2.init(SDL2::INIT_VIDEO|SDL2::INIT_EVENTS)

window = SDL2::Window.create("read_pixels",
                             SDL2::Window::POS_CENTERED, SDL2::Window::POS_CENTERED,
                             320, 240, 0)
renderer = window.create_renderer(-1, 0)

# Draw something
renderer.draw_color = [0, 128, 255]
renderer.fill_rect(SDL2::Rect.new(60, 40, 200, 160))
renderer.draw_color = [255, 255, 0]
renderer.fill_rect(SDL2::Rect.new(110, 80, 100, 80))

# Capture pixels in ARGB8888 format
pixels = renderer.read_pixels(nil, SDL2::PixelFormat::ARGB8888)

renderer.present

# Save as BMP
width, height = 320, 240
row_size = width * 4
file_size = 54 + row_size * height
bmp = String.new(encoding: Encoding::BINARY)
bmp << ["BM", file_size, 0, 0, 54].pack("A2Vv2V")
bmp << [40, width, height, 1, 32, 0, 0, 0, 0, 0, 0].pack("VV2v2V6")
(height - 1).downto(0) {|y| bmp << pixels.byteslice(y * row_size, row_size) }
File.binwrite("screenshot.bmp", bmp)
puts "Saved screenshot.bmp (#{File.size("screenshot.bmp")} bytes)"

loop do
  while ev = SDL2::Event.poll
    if SDL2::Event::KeyDown === ev && ev.scancode == SDL2::Key::Scan::ESCAPE
      exit
    end
  end

  renderer.present
  sleep 0.1
end
