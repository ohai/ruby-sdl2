#include "rubysdl2_internal.h"
#include <SDL_video.h>
#include <SDL_version.h>

static VALUE mGL;
static VALUE cGLContext;

static VALUE current_context = Qnil;

typedef struct GLContext {
    SDL_GLContext context;
} GLContext;

DEFINE_WRAPPER(SDL_GLContext, GLContext, context, cGLContext, "SDL2::GL::Context");

static void GLContext_free(GLContext* c)
{
    if (c->context)
        SDL_GL_DeleteContext(c->context);
    free(c);
}

static VALUE GLContext_new(SDL_GLContext context)
{
    GLContext* c = ALLOC(GLContext);
    c->context = context;
    return Data_Wrap_Struct(cGLContext, 0, GLContext_free, c);
}
/*
 * @overload create(window)
 *   Create an OpenGL context for use with an OpenGL window, and make it
 *   current.
 *
 *   @param window [SDL2::Window] the window corresponding with a new context
 *   @return [SDL2::GL::Context]
 *
 *   @see #delete
 */
static VALUE GLContext_s_create(VALUE self, VALUE window)
{
    SDL_GLContext context = SDL_GL_CreateContext(Get_SDL_Window(window));
    if (!context)
        SDL_ERROR();

    return (current_context = GLContext_new(context));
}

static VALUE GLContext_destroy(VALUE self)
{
    GLContext* c = Get_GLContext(self);
    if (c->context)
        SDL_GL_DeleteContext(c->context);
    c->context = NULL;
    return Qnil;
}

static VALUE GLContext_make_current(VALUE self, VALUE window)
{
    HANDLE_ERROR(SDL_GL_MakeCurrent(Get_SDL_Window(window), Get_SDL_GLContext(self)));
    current_context = self;
    return Qnil;
}

static VALUE GLContext_s_current(VALUE self)
{
    return current_context;
}

static VALUE GL_s_extension_supported_p(VALUE self, VALUE extension)
{
    return INT2BOOL(SDL_GL_ExtensionSupported(StringValueCStr(extension)));
}

static VALUE GL_s_swap_interval(VALUE self)
{
    return INT2NUM(SDL_GL_GetSwapInterval());
}

static VALUE GL_s_set_swap_interval(VALUE self, VALUE interval)
{
    HANDLE_ERROR(SDL_GL_SetSwapInterval(NUM2INT(interval)));
    return Qnil;
}

static VALUE GL_s_get_attribute(VALUE self, VALUE attr)
{
    int value;
    HANDLE_ERROR(SDL_GL_GetAttribute(NUM2INT(attr), &value));
    return INT2NUM(value);
}

static VALUE GL_s_set_attribute(VALUE self, VALUE attr, VALUE value)
{
    HANDLE_ERROR(SDL_GL_SetAttribute(NUM2INT(attr), NUM2INT(value)));
    return value;
}

void rubysdl2_init_gl(void)
{
    mGL = rb_define_module_under(mSDL2, "GL");
    cGLContext = rb_define_class_under(mGL, "Context", rb_cObject);

    rb_define_singleton_method(cGLContext, "create", GLContext_s_create, 1);
    rb_define_singleton_method(cGLContext, "current", GLContext_s_current, 0);
    
    rb_define_method(cGLContext, "destroy?", GLContext_destroy_p, 0);
    rb_define_method(cGLContext, "destroy", GLContext_destroy, 0);
    rb_define_method(cGLContext, "make_current", GLContext_make_current, 1);
    
    rb_define_module_function(mGL, "extension_supported?", GL_s_extension_supported_p, 1);
    rb_define_module_function(mGL, "swap_interval", GL_s_swap_interval, 0);
    rb_define_module_function(mGL, "swap_interval=", GL_s_set_swap_interval, 1);
    rb_define_module_function(mGL, "get_attribute", GL_s_get_attribute, 1);
    rb_define_module_function(mGL, "set_attribute", GL_s_set_attribute, 2);
    
    rb_gc_register_address(&current_context);
}
