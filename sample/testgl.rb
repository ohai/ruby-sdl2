require 'sdl2'
require 'gl'

include Gl

WINDOW_W = 640
WINDOW_H = 480

shadedCube = true

SDL2.init(SDL2::INIT_EVERYTHING)
SDL2::GL.set_attribute(SDL2::GL::RED_SIZE, 8)
SDL2::GL.set_attribute(SDL2::GL::GREEN_SIZE, 8)
SDL2::GL.set_attribute(SDL2::GL::BLUE_SIZE, 8)
SDL2::GL.set_attribute(SDL2::GL::ALPHA_SIZE, 8)
SDL2::GL.set_attribute(SDL2::GL::DOUBLEBUFFER, 1)

window = SDL2::Window.create("testgl", 0, 0, WINDOW_W, WINDOW_H, SDL2::Window::OPENGL)
context = SDL2::GL::Context.create(window)

printf("OpenGL version %d.%d\n",
       SDL2::GL.get_attribute(SDL2::GL::CONTEXT_MAJOR_VERSION),
       SDL2::GL.get_attribute(SDL2::GL::CONTEXT_MINOR_VERSION))

glViewport( 0, 0, 640, 400 )
glMatrixMode( GL_PROJECTION )
glLoadIdentity( )

glMatrixMode( GL_MODELVIEW )
glLoadIdentity( )

glEnable(GL_DEPTH_TEST)

glDepthFunc(GL_LESS)

glShadeModel(GL_SMOOTH)

color =
  [[ 1.0,  1.0,  0.0], 
   [ 1.0,  0.0,  0.0],
   [ 0.0,  0.0,  0.0],
   [ 0.0,  1.0,  0.0],
   [ 0.0,  1.0,  1.0],
   [ 1.0,  1.0,  1.0],
   [ 1.0,  0.0,  1.0],
   [ 0.0,  0.0,  1.0]]

cube =
  [[ 0.5,  0.5, -0.5], 
   [ 0.5, -0.5, -0.5],
   [-0.5, -0.5, -0.5],
   [-0.5,  0.5, -0.5],
   [-0.5,  0.5,  0.5],
   [ 0.5,  0.5,  0.5],
   [ 0.5, -0.5,  0.5],
   [-0.5, -0.5,  0.5]]


loop do

  while event = SDL2::Event.poll
    case event
    when SDL2::Event::Quit, SDL2::Event::KeyDown
      exit
    end
  end

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


  glBegin(GL_QUADS) 

  if shadedCube then
    glColor(color[0])
    glVertex(cube[0])
    glColor(color[1])
    glVertex(cube[1])
    glColor(color[2])
    glVertex(cube[2])
    glColor(color[3])
    glVertex(cube[3])
    
    glColor(color[3])
    glVertex(cube[3])
    glColor(color[4])
    glVertex(cube[4])
    glColor(color[7])
    glVertex(cube[7])
    glColor(color[2])
    glVertex(cube[2])
    
    glColor(color[0])
    glVertex(cube[0])
    glColor(color[5])
    glVertex(cube[5])
    glColor(color[6])
    glVertex(cube[6])
    glColor(color[1])
    glVertex(cube[1])
    
    glColor(color[5])
    glVertex(cube[5])
    glColor(color[4])
    glVertex(cube[4])
    glColor(color[7])
    glVertex(cube[7])
    glColor(color[6])
    glVertex(cube[6])
    
    glColor(color[5])
    glVertex(cube[5])
    glColor(color[0])
    glVertex(cube[0])
    glColor(color[3])
    glVertex(cube[3])
    glColor(color[4])
    glVertex(cube[4])
    
    glColor(color[6])
    glVertex(cube[6])
    glColor(color[1])
    glVertex(cube[1])
    glColor(color[2])
    glVertex(cube[2])
    glColor(color[7])
    glVertex(cube[7])
    
  else
    glColor(1.0, 0.0, 0.0)
    glVertex(cube[0])
    glVertex(cube[1])
    glVertex(cube[2])
    glVertex(cube[3])
    
    glColor(0.0, 1.0, 0.0)
    glVertex(cube[3])
    glVertex(cube[4])
    glVertex(cube[7])
    glVertex(cube[2])
    
    glColor(0.0, 0.0, 1.0)
    glVertex(cube[0])
    glVertex(cube[5])
    glVertex(cube[6])
    glVertex(cube[1])
    
    glColor(0.0, 1.0, 1.0)
    glVertex(cube[5])
    glVertex(cube[4])
    glVertex(cube[7])
    glVertex(cube[6])
    
    glColor(1.0, 1.0, 0.0)
    glVertex(cube[5])
    glVertex(cube[0])
    glVertex(cube[3])
    glVertex(cube[4])
    
    glColor(1.0, 0.0, 1.0)
    glVertex(cube[6])
    glVertex(cube[1])
    glVertex(cube[2])
    glVertex(cube[7])
    
  end

  glEnd()
  
  glMatrixMode(GL_MODELVIEW)
  glRotate(5.0, 1.0, 1.0, 1.0)
  
  window.gl_swap

end
