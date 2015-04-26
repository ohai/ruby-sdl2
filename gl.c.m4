/* -*- mode: C -*- */
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
 * Document-module: SDL2::GL
 *
 * This module provides the initialize/shutdown functions of OpenGL.
 *
 */

/*
 * Document-class: SDL2::GL::Context
 *
 * This class represents an OpenGL context.
 *
 * You must create a new OpenGL context before using
 * OpenGL functions.
 *
 * @!method destroy?
 *   Return true if the context is {#destroy destroyed}.
 */

/*
 * @overload create(window)
 *   Create an OpenGL context for use with an OpenGL window, and make it
 *   current.
 *
 *   @param window [SDL2::Window] the window associate with a new context
 *   @return [SDL2::GL::Context]
 *
 *   @see #delete
 *
 *   @example
 *
 *     SDL2.init(SDL2::INIT_EVERYTHING)
 *     # You need to create a window with `OPENGL' flag
 *     window = SDL2::Window.create("testgl", 0, 0, WINDOW_W, WINDOW_H,
 *                                  SDL2::Window::Flags::OPENGL)
 *                                  
 *     # Create a OpenGL context attached to the window
 *     context = SDL2::GL::Context.create(window)
 *
 *     # You can use OpenGL functions
 *          :
 *          
 *     # Delete the context after using OpenGL functions
 *     context.destroy
 */
static VALUE GLContext_s_create(VALUE self, VALUE window)
{
    SDL_GLContext context = SDL_GL_CreateContext(Get_SDL_Window(window));
    if (!context)
        SDL_ERROR();

    return (current_context = GLContext_new(context));
}

/*
 * Delete the OpenGL context.
 *
 * @return [nil]
 *
 * @see #destroy?
 */
static VALUE GLContext_destroy(VALUE self)
{
    GLContext* c = Get_GLContext(self);
    if (c->context)
        SDL_GL_DeleteContext(c->context);
    c->context = NULL;
    return Qnil;
}

/*
 * @overload make_current(window)
 *   Set the OpenGL context for rendering into an OpenGL window.
 *
 *   @param window [SDL2::Window] the window to associate with the context
 *   @return [nil]
 */
static VALUE GLContext_make_current(VALUE self, VALUE window)
{
    HANDLE_ERROR(SDL_GL_MakeCurrent(Get_SDL_Window(window), Get_SDL_GLContext(self)));
    current_context = self;
    return Qnil;
}

/*
 * Get the current OpenGL context.
 *
 * @return [SDL2::GL::Context] the curren context
 * @return [nil] if there is no current context
 *
 * @see #make_current
 */
static VALUE GLContext_s_current(VALUE self)
{
    return current_context;
}

/*
 * @overload extension_supported?(extension) 
 *   Return true if the current context supports **extension**
 *
 *   @param extension [String] the name of an extension
 *   @example
 *     SDL2::GL.extension_supported?("GL_EXT_framebuffer_blit")
 */
static VALUE GL_s_extension_supported_p(VALUE self, VALUE extension)
{
    return INT2BOOL(SDL_GL_ExtensionSupported(StringValueCStr(extension)));
}

/*
 * Get the state of swap interval of the current context.
 *
 * @return [Integer]
 *   return 0 when vsync is not used,
 *   return 1 when vsync is used,
 *   and return -1 when vsync is not supported by the system (OS).
 *   
 */
static VALUE GL_s_swap_interval(VALUE self)
{
    return INT2NUM(SDL_GL_GetSwapInterval());
}


/*
 * @overload swap_interval=(interval) 
 *   Set the state of swap interval of the current context.
 *
 *   @param interval [Integer]
 *     0 if you don't want to wait for vsync,
 *     1 if you want to wait for vsync,
 *     -1 if you want to use late swap tearing
 *   @return [nil]
 *   
 */
static VALUE GL_s_set_swap_interval(VALUE self, VALUE interval)
{
    HANDLE_ERROR(SDL_GL_SetSwapInterval(NUM2INT(interval)));
    return Qnil;
}

/*
 * @overload get_attribute(attr)
 *   Get the acutal value for an attribute from current context.
 *   
 *   @param attr [Integer] the OpenGL attribute to query
 *   @return [Integer] 
 */
static VALUE GL_s_get_attribute(VALUE self, VALUE attr)
{
    int value;
    HANDLE_ERROR(SDL_GL_GetAttribute(NUM2INT(attr), &value));
    return INT2NUM(value);
}

/*
 * @overload set_attribute(attr, value)
 *   Set an OpenGL window attribute before window creation.
 *   
 *   @param attr [Integer] the OpenGL attribute to set
 *   @param value [Integer] the desired value for the attribute
 *   @return [value]
 */
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

    /* define(`DEFINE_GL_ATTR_CONST',`rb_define_const(mGL, "$1", INT2NUM(SDL_GL_$1))') */
    /* OpenGL attribute - minimal bits of red channel in color buffer, default is 3 */
    DEFINE_GL_ATTR_CONST(RED_SIZE);
    /* OpenGL attribute - minimal bits of green channel in color buffer, default is 3 */
    DEFINE_GL_ATTR_CONST(GREEN_SIZE);
    /* OpenGL attribute - minimal bits of blue channel in color buffer, default is 2 */
    DEFINE_GL_ATTR_CONST(BLUE_SIZE);
    /* OpenGL attribute - minimal bits of alpha channel in color buffer, default is 0 */
    DEFINE_GL_ATTR_CONST(ALPHA_SIZE);
    /* OpenGL attribute - minimal bits of framebufer, default is 0 */
    DEFINE_GL_ATTR_CONST(BUFFER_SIZE);
    /* OpenGL attribute - whether the single buffer (0) or double buffer (1), default
       is double buffer */
    DEFINE_GL_ATTR_CONST(DOUBLEBUFFER);
    /* OpenGL attribute - bits of depth buffer, default is 16 */
    DEFINE_GL_ATTR_CONST(DEPTH_SIZE);
    /* OpenGL attribute - bits of stencil buffer, default is 0 */
    DEFINE_GL_ATTR_CONST(STENCIL_SIZE);
    /* OpenGL attribute - minimal bits of red channel in accumlation buffer,
       default is 0 */
    DEFINE_GL_ATTR_CONST(ACCUM_RED_SIZE);
    /* OpenGL attribute - minimal bits of green channel in accumlation buffer,
       default is 0 */
    DEFINE_GL_ATTR_CONST(ACCUM_GREEN_SIZE);
    /* OpenGL attribute - minimal bits of blue channel in accumlation buffer,
       default is 0 */
    DEFINE_GL_ATTR_CONST(ACCUM_BLUE_SIZE);
    /* OpenGL attribute - minimal bits of alpha channel in accumlation buffer,
       default is 0 */
    DEFINE_GL_ATTR_CONST(ACCUM_ALPHA_SIZE);
    /* OpenGL attribute - whether output is stereo (1) or not (0), default is 0 */
    DEFINE_GL_ATTR_CONST(STEREO);
    /* OpenGL attribuite - the number of buffers used for multisampe anti-aliasing,
       default is 0 */
    DEFINE_GL_ATTR_CONST(MULTISAMPLEBUFFERS);
    /* OpenGL attribute - the number of samples used around the current pixel
       use for multisample anti-aliasing, default is 0 */
    DEFINE_GL_ATTR_CONST(MULTISAMPLESAMPLES);
    /* OpenGL attribute - 1 for requiring hardware acceleration, 0 for software rendering,
       default is allowing either */
    DEFINE_GL_ATTR_CONST(ACCELERATED_VISUAL);
    /* OpenGL attribute - not used (deprecated) */
    DEFINE_GL_ATTR_CONST(RETAINED_BACKING);
    /* OpenGL attribute - OpenGL context major version */
    DEFINE_GL_ATTR_CONST(CONTEXT_MAJOR_VERSION);
    /* OpenGL attribute - OpenGL context minor version */
    DEFINE_GL_ATTR_CONST(CONTEXT_MINOR_VERSION);
    /*
     * INT2NUM(SDL_GL_CONTEXT_FLAGS):
     * 
     * OpenGL attribute - the bit combination of following constants, or 0.
     * default is 0
     *
     * * {SDL2::GL::CONTEXT_DEBUG_FLAG}
     * * {SDL2::GL::CONTEXT_FORWARD_COMPATIBLE_FLAG}
     * * {SDL2::GL::CONTEXT_ROBUST_ACCESS_FLAG}
     * * {SDL2::GL::CONTEXT_RESET_ISOLATION_FLAG}
     *
     * These flags are mapped to some OpenGL extensions. Please see
     * the documentation of each constant for more details.
     *
     * https://wiki.libsdl.org/SDL_GLcontextFlag
     */
    DEFINE_GL_ATTR_CONST(CONTEXT_FLAGS);
    /* INT2NUM(SDL_GL_CONTEXT_PROFILE_MASK):
     *
     * OpenGL attribute - type of GL context, one of the following constants,
     * defaults depends on platform
     *
     * * {CONTEXT_PROFILE_CORE}
     * * {CONTEXT_PROFILE_COMPATIBILITY}
     * * {CONTEXT_PROFILE_ES}
     *
     * https://wiki.libsdl.org/SDL_GLprofile
     */
    DEFINE_GL_ATTR_CONST(CONTEXT_PROFILE_MASK);
    /* OpenGL attribute - OpenGL context sharing, default is 0 */
    DEFINE_GL_ATTR_CONST(SHARE_WITH_CURRENT_CONTEXT);
#if SDL_VERSION_ATLEAST(2,0,1)
    /* OpenGL attribute - 1 for requesting sRGB capable visual, default to 0 */
    DEFINE_GL_ATTR_CONST(FRAMEBUFFER_SRGB_CAPABLE);
#endif
    /* OpenGL attribute - not used (deprecated) */
    DEFINE_GL_ATTR_CONST(CONTEXT_EGL);

    /* define(`DEFINE_GL_CONTEXT_CONST',`rb_define_const(mGL, "CONTEXT_$1", INT2NUM(SDL_GL_CONTEXT_$1))') */

    /* This flag maps to GLX_CONTEXT_DEBUG_BIT_ARB in
     * the GLX_ARB_create_context extension for X11
     * and WGL_CONTEXT_DEBUG_BIT_ARB in the WGL_ARB_create_context
     * extension for Windows.
     */
    DEFINE_GL_CONTEXT_CONST(DEBUG_FLAG);
    /*
     * This flag maps to GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB in the
     * GLX_ARB_create_context extension for X11 and
     * WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB in the WGL_ARB_create_context
     * extension for Windows.
     */
    DEFINE_GL_CONTEXT_CONST(FORWARD_COMPATIBLE_FLAG);
    /*
     * This flag maps to GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB in the
     * GLX_ARB_create_context_robustness extension for X11 and
     * WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB in the WGL_ARB_create_context_robustness
     * extension for Windows. 
     */
    DEFINE_GL_CONTEXT_CONST(ROBUST_ACCESS_FLAG);
    /*
     * This flag maps to GLX_CONTEXT_RESET_ISOLATION_BIT_ARB in the
     * GLX_ARB_robustness_isolation extension for X11 and
     * WGL_CONTEXT_RESET_ISOLATION_BIT_ARB in the WGL_ARB_create_context_robustness
     * extension for Windows.
     */
    DEFINE_GL_CONTEXT_CONST(RESET_ISOLATION_FLAG);

    /*
     * OpenGL core profile - deprecated
     * functions are disabled
     */
    DEFINE_GL_CONTEXT_CONST(PROFILE_CORE);
    /*
     * OpenGL compatibility profile -
     * deprecated functions are allowed
     */
    DEFINE_GL_CONTEXT_CONST(PROFILE_COMPATIBILITY);
    /*
     * OpenGL ES profile - only a subset of the
     * base OpenGL functionality is available
     */
    DEFINE_GL_CONTEXT_CONST(PROFILE_ES);
    
    rb_gc_register_address(&current_context);
}
