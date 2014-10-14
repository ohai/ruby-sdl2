#include <ruby.h>
#define SDL_MAIN_HANDLED
#include <SDL_surface.h>
#include <SDL_version.h>
#include <SDL_video.h>

#ifndef SDL2_EXTERN
#define SDL2_EXTERN extern
#endif

/** utility functions */
int rubysdl2_handle_error(int code, const char* cfunc);
int rubysdl2_is_active(void);
void rubysdl2_define_attr_readers(VALUE klass, ...);
VALUE rubysdl2_utf8str_new_cstr(const char* str);
VALUE rubysdl2_Surface_new(SDL_Surface* surface);
SDL_Color rubysdl2_Array_to_SDL_Color(VALUE ary);
VALUE rubysdl2_SDL_version_to_String(const SDL_version* ver);
VALUE rubysdl2_SDL_version_to_Array(const SDL_version* ver);
VALUE rubysdl2_find_window_by_id(Uint32 id);
SDL_Rect* rubysdl2_Get_SDL_Rect(VALUE);
SDL_Window* rubysdl2_Get_SDL_Window(VALUE);
const char* rubysdl2_INT2BOOLCSTR(int);

/** initialize interfaces */
void rubysdl2_init_video(void);
void rubysdl2_init_event(void);
void rubysdl2_init_key(void);
void rubysdl2_init_mouse(void);
void rubysdl2_init_joystick(void);
void rubysdl2_init_timer(void);
void rubysdl2_init_image(void);
void rubysdl2_init_mixer(void);
void rubysdl2_init_ttf(void);

/** macros */
#define HANDLE_ERROR(c) (rubysdl2_handle_error((c), __func__))
#define SDL_ERROR() (HANDLE_ERROR(-1))
#define INT2BOOL(x) ((x)?Qtrue:Qfalse)
#define NUM2UCHAR NUM2UINT
#define UCHAR2NUM UINT2NUM

#define DEFINE_GETTER(scope, ctype, var_class, classname)               \
    scope ctype* Get_##ctype(VALUE obj)                                 \
    {                                                                   \
        ctype* s;                                                       \
        if (!rb_obj_is_kind_of(obj, var_class))                         \
            rb_raise(rb_eTypeError, "wrong argument type %s (expected %s)", \
                     rb_obj_classname(obj), classname);                 \
        Data_Get_Struct(obj, ctype, s);                                 \
                                                                        \
        return s;                                                       \
    }

#define DEFINE_WRAP_GETTER(scope, SDL_typename, struct_name, field, classname) \
    scope SDL_typename* Get_##SDL_typename(VALUE obj)                   \
    {                                                                   \
        struct_name* s = Get_##struct_name(obj);                        \
            if (s->field == NULL)                                       \
                HANDLE_ERROR(SDL_SetError(classname " is already destroyed")); \
                                                                        \
            return s->field;                                            \
    }

#define DEFINE_DESTROY_P(scope, struct_name, field)                     \
    scope VALUE struct_name##_destroy_p(VALUE self)                     \
    {                                                                   \
        return INT2BOOL(Get_##struct_name(self)->field == NULL);        \
    }

#define DEFINE_WRAPPER(SDL_typename, struct_name, field, var_class, classname) \
    DEFINE_GETTER(static, struct_name, var_class, classname);           \
    DEFINE_WRAP_GETTER(static ,SDL_typename, struct_name, field, classname); \
    DEFINE_DESTROY_P(static, struct_name, field);

/** classes and modules */
SDL2_EXTERN VALUE rubysdl2_mSDL2;
SDL2_EXTERN VALUE rubysdl2_eSDL2Error;

/** prefix macros */
#define define_attr_readers rubysdl2_define_attr_readers
#define utf8str_new_cstr rubysdl2_utf8str_new_cstr
#define Surface_new rubysdl2_Surface_new
#define Get_SDL_Rect rubysdl2_Get_SDL_Rect
#define Get_SDL_Window rubysdl2_Get_SDL_Window
#define Array_to_SDL_Color rubysdl2_Array_to_SDL_Color 
#define mSDL2 rubysdl2_mSDL2
#define eSDL2Error rubysdl2_eSDL2Error
#define SDL_version_to_String rubysdl2_SDL_version_to_String
#define SDL_version_to_Array rubysdl2_SDL_version_to_Array
#define INT2BOOLCSTR  rubysdl2_INT2BOOLCSTR 
#define find_window_by_id rubysdl2_find_window_by_id


