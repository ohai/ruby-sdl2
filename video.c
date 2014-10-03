#include "rubysdl2_internal.h"
#include <SDL_video.h>

#include <ruby/encoding.h>

static VALUE mWindow;
// static VALUE mRenderer;
// static VALUE mTexture;

#define DEFINE_WRAP_STRUCT(SDL_typename, struct_name, field)    \
  typedef struct {                                              \
    SDL_typename* field;                                        \
  } struct_name;                                  

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

#define DEFINE_NEW(SDL_typename, struct_name, field, var_class)         \
  static void struct_name##_free(struct_name* s);                       \
  static VALUE struct_name##_new(SDL_typename* data)                    \
  {                                                                     \
    struct_name* s = ALLOC(struct_name);                                \
    s->field = data;                                                    \
    return Data_Wrap_Struct(var_class, 0, struct_name##_free, s);       \
  }

#define DEFINE_DESTROY_P(struct_name, field)                    \
  static VALUE struct_name##_destroy_p(VALUE self)              \
  {                                                             \
    return INT2BOOL(Get_##struct_name(self)->field == NULL);    \
  }

#define DEFINE_DESTROYABLE(SDL_typename, struct_name, field, var_class, classname) \
  DEFINE_WRAP_STRUCT(SDL_typename, struct_name, field);                 \
  DEFINE_GETTER(struct_name, var_class, classname);                     \
  DEFINE_WRAP_GETTER(SDL_typename, struct_name, field, classname);      \
  DEFINE_NEW(SDL_typename, struct_name, field, var_class);              \
  DEFINE_DESTROY_P(struct_name, field);


DEFINE_DESTROYABLE(SDL_Window, Window, window, mWindow, "SDL2::Window");
static void Window_free(Window* w)
{                                 
  if (w->window != NULL)
    SDL_DestroyWindow(w->window);
  free(w);
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
  rb_define_method(mWindow, "destroy?", Window_destroy_p, 0);
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
  
