#include "rubysdl2_internal.h"
#include <SDL_video.h>
#include <SDL_render.h>
#include <ruby/encoding.h>

static VALUE cWindow;
static VALUE cRenderer;
/* static VALUE mTexture; */

#define DEFINE_GETTER(ctype, var_class, classname)                      \
  ctype* Get_##ctype(VALUE obj)                                         \
  {                                                                     \
    ctype* s;                                                           \
    if (!rb_obj_is_kind_of(obj, var_class))                             \
      rb_raise(rb_eTypeError, "wrong argument type %s (expected %s)",   \
               rb_obj_classname(obj), classname);                       \
    Data_Get_Struct(obj, ctype, s);                                     \
                                                                        \
    return s;                                                           \
  }

#define DEFINE_WRAP_GETTER(SDL_typename, struct_name, field, classname) \
  SDL_typename* Get_##SDL_typename(VALUE obj)                           \
  {                                                                     \
    struct_name* s = Get_##struct_name(obj);                            \
      if (s->field == NULL)                                             \
        HANDLE_ERROR(SDL_SetError(classname " is already destroyed"));  \
                                                                        \
      return s->field;                                                  \
  }

#define DEFINE_DESTROY_P(struct_name, field)                    \
  static VALUE struct_name##_destroy_p(VALUE self)              \
  {                                                             \
    return INT2BOOL(Get_##struct_name(self)->field == NULL);    \
  }

#define DEFINE_WRAPPER(SDL_typename, struct_name, field, var_class, classname) \
  DEFINE_GETTER(struct_name, var_class, classname);                     \
  DEFINE_WRAP_GETTER(SDL_typename, struct_name, field, classname);      \
  DEFINE_DESTROY_P(struct_name, field);


struct Window;
struct Renderer;
struct Texture;

typedef struct Window {
  SDL_Window* window;
  int num_renderers;
  int max_renderers;
  struct Renderer** renderers;
} Window;

typedef struct Renderer {
  SDL_Renderer* renderer;
  int num_textures;
  int max_textures;
  struct Texture** textures;
  int refcount;
} Renderer;

typedef struct Texture {
  SDL_Texture* texture;
  int refcount;
} Texture;

static void Renderer_free(Renderer*);
static void Window_free(Window* w)
{
  int i;
  if (w->window != NULL)
    SDL_DestroyWindow(w->window);
  for (i=0; i<w->num_renderers; ++i) 
    Renderer_free(w->renderers[i]);
  free(w->renderers);
  free(w);
}

static VALUE Window_new(SDL_Window* window)
{
  Window* w = ALLOC(Window);
  w->window = window;
  w->num_renderers = 0;
  w->max_renderers = 4;
  w->renderers = ALLOC_N(struct Renderer*, 4);
  return Data_Wrap_Struct(cWindow, 0, Window_free, w);
}

DEFINE_WRAPPER(SDL_Window, Window, window, cWindow, "SDL2::Window");

static void Renderer_free(Renderer* r)
{
  if (r->renderer != NULL) {
    SDL_DestroyRenderer(r->renderer);
    r->renderer = NULL;
  }
  r->refcount--;
  if (r->refcount == 0) {
    free(r->textures);
    free(r);
  }
}

static VALUE Renderer_new(SDL_Renderer* renderer)
{
  Renderer* r = ALLOC(Renderer);
  r->renderer = renderer;
  r->num_textures = 0;
  r->max_textures = 16;
  r->textures = ALLOC_N(Texture*, 16);
  r->refcount = 1;
  return Data_Wrap_Struct(cRenderer, 0, Renderer_free, r);
}

DEFINE_WRAPPER(SDL_Renderer, Renderer, renderer, cRenderer, "SDL2::Renderer")

static VALUE Window_s_create(VALUE self, VALUE title, VALUE x, VALUE y, VALUE w, VALUE h,
                             VALUE flags)
{
  SDL_Window* window;
  VALUE win;
  title = rb_str_export_to_enc(title, rb_utf8_encoding());
  window = SDL_CreateWindow(StringValueCStr(title),
                            NUM2INT(x), NUM2INT(y), NUM2INT(w), NUM2INT(h),
                            NUM2UINT(flags));
  if (window == NULL)
    HANDLE_ERROR(-1);

  win = Window_new(window);
  return win;
}

static void Window_attach_renderer(Window* w, Renderer* r)
{
  if (w->num_renderers == w->max_renderers) {
    w->max_renderers *= 2;
    REALLOC_N(w->renderers, Renderer*, w->max_renderers);
  }
  w->renderers[w->num_renderers++] = r;
  ++r->refcount;
}

static VALUE Window_create_renderer(VALUE self, VALUE index, VALUE flags)
{
  SDL_Renderer* sdl_renderer;
  VALUE renderer;
  sdl_renderer = SDL_CreateRenderer(Get_SDL_Window(self), NUM2INT(index), NUM2UINT(flags));
  
  if (sdl_renderer == NULL)
    HANDLE_ERROR(-1);
  
  renderer = Renderer_new(sdl_renderer);
  Window_attach_renderer(Get_Window(self), Get_Renderer(renderer));
  return renderer;
}

void rubysdl2_init_video(void)
{
  cWindow = rb_define_class_under(mSDL2, "Window", rb_cObject);

  rb_define_singleton_method(cWindow, "create", Window_s_create, 6);
  rb_define_method(cWindow, "destroy?", Window_destroy_p, 0);
  rb_define_method(cWindow, "create_renderer", Window_create_renderer, 2);
  rb_define_const(cWindow, "OP_CENTERED", INT2NUM(SDL_WINDOWPOS_CENTERED));
  rb_define_const(cWindow, "OP_UNDEFINED", INT2NUM(SDL_WINDOWPOS_UNDEFINED));
#define DEFINE_SDL_WINDOW_FLAGS_CONST(n) \
  rb_define_const(cWindow, #n, UINT2NUM(SDL_WINDOW_##n));
  DEFINE_SDL_WINDOW_FLAGS_CONST(FULLSCREEN);
  DEFINE_SDL_WINDOW_FLAGS_CONST(FULLSCREEN_DESKTOP);
  DEFINE_SDL_WINDOW_FLAGS_CONST(OPENGL);
  DEFINE_SDL_WINDOW_FLAGS_CONST(SHOWN);
  DEFINE_SDL_WINDOW_FLAGS_CONST(HIDDEN);
  DEFINE_SDL_WINDOW_FLAGS_CONST(BORDERLESS);
  DEFINE_SDL_WINDOW_FLAGS_CONST(RESIZABLE);
  DEFINE_SDL_WINDOW_FLAGS_CONST(MINIMIZED);
  DEFINE_SDL_WINDOW_FLAGS_CONST(MAXIMIZED);
  DEFINE_SDL_WINDOW_FLAGS_CONST(INPUT_GRABBED);
  DEFINE_SDL_WINDOW_FLAGS_CONST(INPUT_FOCUS);
  DEFINE_SDL_WINDOW_FLAGS_CONST(MOUSE_FOCUS);
  DEFINE_SDL_WINDOW_FLAGS_CONST(FOREIGN);
#ifdef SDL_WINDOW_ALLOW_HIGHDPI
  DEFINE_SDL_WINDOW_FLAGS_CONST(ALLOW_HIGHDPI);
#endif
#undef DEFINE_SDL_WINDOW_FLAGS_CONST

  cRenderer = rb_define_class_under(mSDL2, "Renderer", rb_cObject);

  rb_define_method(cRenderer, "destroy?", Renderer_destroy_p, 0);
}
  
