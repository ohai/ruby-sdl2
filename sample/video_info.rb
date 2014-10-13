require 'sdl2'
require "pp"

SDL2.init(SDL2::INIT_VIDEO)

pp SDL2::PixelFormat::FORMATS.map{|f| f.name}
p SDL2::PixelFormat::RGBA8888

p SDL2::Display.displays
SDL2::Display.displays.each{|display| p display.modes }
display = SDL2::Display.displays.first
print "curent mode: "; p display.current_mode
print "desktop mode: "; p display.desktop_mode
search_mode = SDL2::Display::Mode.new(SDL2::PixelFormat::UNKNOWN, 640, 480, 60)
puts "The mode closest to #{search_mode.inspect} is #{display.closest_mode(search_mode).inspect}"
print "bounds: "; p display.bounds
puts "current video driver: #{SDL2.current_video_driver}"

window = SDL2::Window.create("testsprite", 0, 0, 640, 480, 0)
renderer = window.create_renderer(-1, 0)
texture = renderer.load_texture("icon.bmp")

puts "window id: #{window.window_id}"
p SDL2::Window.all_windows
p window.display_mode
p window.display

p SDL2.video_drivers
p SDL2::Renderer.drivers_info
p renderer.info
p renderer.clip_rect
p renderer.logical_size
p renderer.scale
p renderer.viewport
p renderer.support_render_target?
p renderer.output_size

p renderer.info.texture_formats
renderer.info.texture_formats.each do |format|
  p [format.format, format.name, format.type,  format.order, format.layout, format.bps, format.bytes_per_pixel,
     format.indexed?, format.alpha?, format.fourcc?]

end

p texture
texture.destroy
p texture

renderer.present

