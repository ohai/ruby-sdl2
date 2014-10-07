#include <ruby.h>

#define SDL_MAIN_HANDLED

#ifndef SDL2_EXTERN
#define SDL2_EXTERN extern
#endif

/** utility functions */
int rubysdl2_handle_error(int code, const char* cfunc);
int rubysdl2_is_active(void);
void rubysdl2_define_attr_readers(VALUE klass, ...);

/** initialize interfaces */
void rubysdl2_init_video(void);
void rubysdl2_init_event(void);
void rubysdl2_init_key(void);
void rubysdl2_init_timer(void);
void rubysdl2_init_image(void);
void rubysdl2_init_mixer(void);

/** macros */
#define HANDLE_ERROR(c) (rubysdl2_handle_error((c), __func__))
#define SDL_ERROR() (HANDLE_ERROR(-1))
#define INT2BOOL(x) ((x)?Qtrue:Qfalse)

#define define_attr_readers rubysdl2_define_attr_readers

#define DEFINE_GETTER(ctype, var_class, classname)                      \
    static ctype* Get_##ctype(VALUE obj)                                \
    {                                                                   \
        ctype* s;                                                       \
        if (!rb_obj_is_kind_of(obj, var_class))                         \
            rb_raise(rb_eTypeError, "wrong argument type %s (expected %s)", \
                     rb_obj_classname(obj), classname);                 \
        Data_Get_Struct(obj, ctype, s);                                 \
                                                                        \
        return s;                                                       \
    }

#define DEFINE_WRAP_GETTER(SDL_typename, struct_name, field, classname) \
    static SDL_typename* Get_##SDL_typename(VALUE obj)                  \
    {                                                                   \
        struct_name* s = Get_##struct_name(obj);                        \
            if (s->field == NULL)                                       \
                HANDLE_ERROR(SDL_SetError(classname " is already destroyed")); \
                                                                        \
            return s->field;                                            \
    }

#define DEFINE_DESTROY_P(struct_name, field)                            \
    static VALUE struct_name##_destroy_p(VALUE self)                    \
    {                                                                   \
        return INT2BOOL(Get_##struct_name(self)->field == NULL);        \
    }

#define DEFINE_WRAPPER(SDL_typename, struct_name, field, var_class, classname) \
    DEFINE_GETTER(struct_name, var_class, classname);                   \
    DEFINE_WRAP_GETTER(SDL_typename, struct_name, field, classname);    \
    DEFINE_DESTROY_P(struct_name, field);

/** classes and modules */
#define mSDL2 rubysdl2_mSDL2
#define eSDL2Error rubysdl2_eSDL2Error

SDL2_EXTERN VALUE mSDL2;
SDL2_EXTERN VALUE eSDL2Error;



