#include "rubysdl2_internal.h"
#include <SDL_video.h>
#include <ruby/encoding.h>

static VALUE mWindow;

typedef struct {
  SDL_Window* window;
} Window;

SDL_Window* Get_SDL_Window(VALUE obj)
{
  Window* w;
  if (!rb_obj_is_kind_of(obj, mWindow))
    rb_raise(rb_eTypeError, "wrong argument type %s (expected SDL2::Window)",
             rb_obj_classname(obj));
  Data_Get_Struct(obj, Window, w);

  if (w->window == NULL) {
    HANDLE_ERROR(SDL_SetError("Wiwndow is already destroyed"));
  }

  return w->window;
}

static void Window_free(Window* w)
{
  if (w->window != NULL)
    SDL_DestroyWindow(w->window);
  free(w);
}

static VALUE Window_new(SDL_Window* window)
{
  Window* w = ALLOC(Window);
  w->window = window;
  return Data_Wrap_Struct(mWindow, 0, Window_free, w);
}

static VALUE Window_s_create(VALUE self, VALUE title, VALUE x, VALUE y, VALUE w, VALUE h,
                             VALUE flags)
{
  SDL_Window* window;
  title = rb_str_export_to_enc(title, rb_utf8_encoding());
  window = SDL_CreateWindow(StringValueCStr(title),
                            NUM2INT(x), NUM2INT(y), NUM2INT(w), NUM2INT(h),
                            NUM2UINT(flags));
  if (window == NULL)
    HANDLE_ERROR(-1);

  return Window_new(window);
}
                             
void rubysdl2_init_video(void)
{
  mWindow = rb_define_class_under(mSDL2, "Window", rb_cObject);

  rb_define_singleton_method(mWindow, "create", Window_s_create, 6);
  rb_define_const(mWindow, "OP_CENTERED", INT2NUM(SDL_WINDOWPOS_CENTERED));
  rb_define_const(mWindow, "OP_UNDEFINED", INT2NUM(SDL_WINDOWPOS_UNDEFINED));
#define DEFINE_SDL_WINDOW_FLAGS_CONST(n) \
  rb_define_const(mWindow, #n, UINT2NUM(SDL_WINDOW_##n));
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
  
}
  
