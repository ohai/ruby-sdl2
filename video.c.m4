/* -*- mode: C -*- */
#include "rubysdl2_internal.h"
#include <SDL_video.h>
#include <SDL_version.h>
#include <SDL_render.h>
#include <SDL_messagebox.h>
#include <SDL_endian.h>
#include <ruby/encoding.h>

static VALUE cWindow;
static VALUE mWindowFlags;
static VALUE cDisplay;
static VALUE cDisplayMode;
static VALUE cRenderer;
static VALUE mRendererFlags;
static VALUE mBlendMode;
static VALUE cTexture;
static VALUE cRect;
static VALUE cPoint;
static VALUE cSurface;
static VALUE cRendererInfo;
static VALUE cPixelFormat; /* NOTE: This is related to SDL_PixelFormatEnum, not SDL_PixelFormat */
static VALUE mPixelType;
static VALUE mBitmapOrder;
static VALUE mPackedOrder;
static VALUE mArrayOrder;
static VALUE mPackedLayout;

static VALUE mScreenSaver;

static VALUE hash_windowid_to_window = Qnil;

struct Window;
struct Renderer;
struct Texture;

#ifdef DEBUG_GC
#define GC_LOG(args) fprintf args
#else
#define GC_LOG(args) 
#endif

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

typedef struct Surface {
    SDL_Surface* surface;
    int need_to_free_pixels;
} Surface;

static void Renderer_free(Renderer*);
static void Window_destroy_internal(Window* w)
{
    int i;
    for (i=0; i<w->num_renderers; ++i) 
        Renderer_free(w->renderers[i]);
    w->num_renderers = w->max_renderers = 0;
    free(w->renderers);
    w->renderers = NULL;
}

static void Window_free(Window* w)
{
    
    GC_LOG((stderr, "Window free: %p\n", w));
    Window_destroy_internal(w);
    if (w->window && rubysdl2_is_active())
        SDL_DestroyWindow(w->window);
    
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

DEFINE_GETTER(static, Window, cWindow, "SDL2::Window");
DEFINE_WRAP_GETTER(, SDL_Window, Window, window, "SDL2::Window");
DEFINE_DESTROY_P(static, Window, window);

static VALUE Display_new(int index)
{
    VALUE display = rb_obj_alloc(cDisplay);
    rb_iv_set(display, "@index", INT2NUM(index));
    rb_iv_set(display, "@name", utf8str_new_cstr(SDL_GetDisplayName(index)));
    return display;
}

static VALUE DisplayMode_s_allocate(VALUE klass)
{
    SDL_DisplayMode* mode = ALLOC(SDL_DisplayMode);
    mode->format = mode->w = mode->h = mode->refresh_rate = 0;
    mode->driverdata = NULL;
    return Data_Wrap_Struct(klass, 0, free, mode);
}

static VALUE DisplayMode_new(SDL_DisplayMode* mode)
{
    VALUE display_mode = DisplayMode_s_allocate(cDisplayMode);
    SDL_DisplayMode* m;
    Data_Get_Struct(display_mode, SDL_DisplayMode, m);
    *m = *mode;
    return display_mode;
}

DEFINE_GETTER(static, SDL_DisplayMode, cDisplayMode, "SDL2::Display::Mode");

static void Texture_free(Texture*);
static void Renderer_destroy_internal(Renderer* r)
{
    int i;
    for (i=0; i<r->num_textures; ++i)
        Texture_free(r->textures[i]);
    free(r->textures);
    r->textures = NULL;
    r->max_textures = r->num_textures = 0;

    if (r->renderer && rubysdl2_is_active()) {
        SDL_DestroyRenderer(r->renderer);
    }
    r->renderer = NULL;
}

static void Renderer_free(Renderer* r)
{
    GC_LOG((stderr, "Renderer free: %p (refcount=%d)\n", r, r->refcount));
    Renderer_destroy_internal(r);
    
    r->refcount--;
    if (r->refcount == 0) {
        free(r);
    }
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

static VALUE Renderer_new(SDL_Renderer* renderer, Window* w)
{
    Renderer* r = ALLOC(Renderer);
    r->renderer = renderer;
    r->num_textures = 0;
    r->max_textures = 16;
    r->textures = ALLOC_N(Texture*, 16);
    r->refcount = 1;
    Window_attach_renderer(w, r);
    return Data_Wrap_Struct(cRenderer, 0, Renderer_free, r);
}

DEFINE_WRAPPER(SDL_Renderer, Renderer, renderer, cRenderer, "SDL2::Renderer");

static void Texture_destroy_internal(Texture* t)
{
    if (t->texture && rubysdl2_is_active()) {
        SDL_DestroyTexture(t->texture);
    }
    t->texture = NULL;
}

static void Texture_free(Texture* t)
{
    GC_LOG((stderr, "Texture free: %p (refcount=%d)\n", t, t->refcount));
    Texture_destroy_internal(t);
    t->refcount--;
    if (t->refcount == 0) {
        free(t);
    }
}

static void Renderer_attach_texture(Renderer* r, Texture* t)
{
    if (r->max_textures == r->num_textures) {
        r->max_textures *= 2;
        REALLOC_N(r->textures, Texture*, r->max_textures);
    }
    r->textures[r->num_textures++] = t;
    ++t->refcount;
}

static VALUE Texture_new(SDL_Texture* texture, Renderer* r)
{
    Texture* t = ALLOC(Texture);
    t->texture = texture;
    t->refcount = 1;
    Renderer_attach_texture(r, t);
    return Data_Wrap_Struct(cTexture, 0, Texture_free, t);
}

DEFINE_WRAPPER(SDL_Texture, Texture, texture, cTexture, "SDL2::Texture");


static void Surface_free(Surface* s)
{
    GC_LOG((stderr, "Surface free: %p\n", s));
    if (s->need_to_free_pixels)
        free(s->surface->pixels);
    if (s->surface && rubysdl2_is_active())
        SDL_FreeSurface(s->surface);
    free(s);
}

VALUE Surface_new(SDL_Surface* surface)
{
    Surface* s = ALLOC(Surface);
    s->surface = surface;
    s->need_to_free_pixels = 0;
    return Data_Wrap_Struct(cSurface, 0, Surface_free, s);
}

DEFINE_WRAPPER(SDL_Surface, Surface, surface, cSurface, "SDL2::Surface");

DEFINE_GETTER(, SDL_Rect, cRect, "SDL2::Rect");

DEFINE_GETTER(static, SDL_Point, cPoint, "SDL2::Point");

static VALUE PixelFormat_new(Uint32 format)
{
    VALUE fmt = UINT2NUM(format);
    return rb_class_new_instance(1, &fmt, cPixelFormat);
}

static VALUE RendererInfo_new(SDL_RendererInfo* info)
{
    VALUE rinfo = rb_obj_alloc(cRendererInfo);
    VALUE texture_formats = rb_ary_new();
    unsigned int i;
    
    rb_iv_set(rinfo, "@name", rb_usascii_str_new_cstr(info->name));
    rb_iv_set(rinfo, "@texture_formats", texture_formats);
    for (i=0; i<info->num_texture_formats; ++i)
        rb_ary_push(texture_formats, PixelFormat_new(info->texture_formats[i]));
    rb_iv_set(rinfo, "@max_texture_width", INT2NUM(info->max_texture_width));
    rb_iv_set(rinfo, "@max_texture_height", INT2NUM(info->max_texture_height));
    
    return rinfo;
}

SDL_Color Array_to_SDL_Color(VALUE ary)
{
    SDL_Color color;
    VALUE a;
    if (ary == Qnil) {
        color.r = color.g = color.b = 0; color.a = 255;
        return color;
    }
        
    Check_Type(ary, T_ARRAY);
    if (RARRAY_LEN(ary) != 3 && RARRAY_LEN(ary) != 4)
        rb_raise(rb_eArgError, "wrong number of Array elements (%ld for 3 or 4)",
                 RARRAY_LEN(ary));
    color.r = NUM2UCHAR(rb_ary_entry(ary, 0));
    color.g = NUM2UCHAR(rb_ary_entry(ary, 1));
    color.b = NUM2UCHAR(rb_ary_entry(ary, 2));
    a = rb_ary_entry(ary, 3);
    if (a == Qnil)
        color.a = 255;
    else
        color.a = NUM2UCHAR(a);
    return color;
}

/*
 * Get the names of all video drivers.
 *
 * You can use the name as an argument of {.video_init}.
 * 
 * @return [Array<String>]
 */
static VALUE SDL2_s_video_drivers(VALUE self)
{
    int num_drivers = HANDLE_ERROR(SDL_GetNumVideoDrivers());
    int i;
    VALUE drivers = rb_ary_new();
    for (i=0; i<num_drivers; ++i)
        rb_ary_push(drivers, rb_usascii_str_new_cstr(SDL_GetVideoDriver(i)));
    return drivers;
}

/*
 * Get the name of current video driver
 *
 * @return [String] the name of the current video driver
 * @return [nil] when the video is not initialized
 */
static VALUE SDL2_s_current_video_driver(VALUE self)
{
    const char* name = SDL_GetCurrentVideoDriver();
    if (name)
        return utf8str_new_cstr(name);
    else
        return Qnil;
}

/*
 * @overload video_init(driver_name) 
 *   Initialize the video subsystem, specifying a video driver.
 *   
 *   {.init} cannot specify a video driver, so you need to use
 *   this method to specify a driver.
 *   
 *   @param driver_name [String]
 *   @return [nil]
 *
 *   @see .init
 */
static VALUE SDL2_s_video_init(VALUE self, VALUE driver_name)
{
    HANDLE_ERROR(SDL_VideoInit(StringValueCStr(driver_name)));
    return Qnil;
}

/*
 * Document-class: SDL2::Window
 * 
 * This class represents a window.
 *
 * If you want to create graphical application using Ruby/SDL, first you need to
 * create a window.
 * 
 * All of methods/class methods are available only after initializing video
 * subsystem by {SDL2.init}.
 *
 *
 * @!method destroy?
 *   Return true if the window is already destroyed.
 */

/*
 * @overload create(title, x, y, w, h, flags)
 *   Create a window with the specified position (x,y), dimensions (w,h) and flags.
 *
 *   @param [Integer] x the x position of the left-top of the window
 *   @param [Integer] y the y position of the left-top of the window
 *   @param [Integer] w the width of the window
 *   @param [Integer] h the height of the window
 *   @param [Integer] flags 0, or one or more {Flags} OR'd together
 *   
 *   @return [SDL2::Window] created window
 *   
 */
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
    rb_hash_aset(hash_windowid_to_window, UINT2NUM(SDL_GetWindowID(window)), win);
    return win;
}

/*
 * Get all windows under SDL.
 *
 * @return [Hash<Integer => SDL2::Window>]
 *   the hash from window id to the {SDL2::Window} objects.
 */
static VALUE Window_s_all_windows(VALUE self)
{
    return rb_hash_dup(hash_windowid_to_window);
}

/*
 * @overload find_by_id(id)
 *   Get the window from ID.
 *
 *   @param id [Integer] the window id you want to find
 *   @return [SDL2::Window] the window associated with **id**
 *   @return [nil] when no window is associated with **id**
 *
 */
static VALUE Window_s_find_by_id(VALUE self, VALUE id)
{
    return rb_hash_aref(hash_windowid_to_window, id);
}

VALUE find_window_by_id(Uint32 id)
{
    return Window_s_find_by_id(Qnil, UINT2NUM(id));
}

/*
 * @overload destroy
 *   Destroy window.
 *
 *   You cannot call almost all methods after calling this method.
 *   The exception is {#destroy?}.
 *   
 *   @return [void]
 */
static VALUE Window_destroy(VALUE self)
{
    Window_destroy_internal(Get_Window(self));
    return Qnil;
}

/*
 * @overload create_renderer(index, flags)
 *   Create a 2D rendering context for a window.
 *
 *   @param [Integer] index the index of the rendering driver to initialize,
 *     or -1 to initialize the first one supporting the requested flags
 *   @param [Integer] flags 0, or one or more [Renderer flag masks](SDL2) OR'd together;
 *
 *   @return [SDL2::Renderer] the created renderer (rendering context)
 */
static VALUE Window_create_renderer(VALUE self, VALUE index, VALUE flags)
{
    SDL_Renderer* sdl_renderer;
    VALUE renderer;
    sdl_renderer = SDL_CreateRenderer(Get_SDL_Window(self), NUM2INT(index), NUM2UINT(flags));
  
    if (sdl_renderer == NULL)
        HANDLE_ERROR(-1);
  
    renderer = Renderer_new(sdl_renderer, Get_Window(self));
    rb_iv_set(self, "renderer", renderer);
    return renderer;
}

/*
 * @overload renderer
 *   Return the renderer associate with the window
 *
 *   @return [SDL2::Renderer] the associated renderer
 *   @return [nil] if no renderer is created yet
 */
static VALUE Window_renderer(VALUE self)
{
    return rb_iv_get(self, "renderer");
}

/*
 * Get the numeric ID of the window.
 *
 * @return [Integer]
 */
static VALUE Window_window_id(VALUE self)
{
    return UINT2NUM(SDL_GetWindowID(Get_SDL_Window(self)));
}

/*
 * Get information about the window.
 *
 * @return [SDL2::Window::Mode]
 */
static VALUE Window_display_mode(VALUE self)
{
    SDL_DisplayMode mode;
    HANDLE_ERROR(SDL_GetWindowDisplayMode(Get_SDL_Window(self), &mode));
    return DisplayMode_new(&mode);
}

/*
 * Get the display associated with the window.
 *
 * @return [SDL2::Display]
 */
static VALUE Window_display(VALUE self)
{
    int display_index = HANDLE_ERROR(SDL_GetWindowDisplayIndex(Get_SDL_Window(self)));
    return Display_new(display_index);
}

/*
 * Get the brightness (gamma correction) of the window.
 *
 * @return [Float] the brightness
 * @see #brightness=
 */
static VALUE Window_brightness(VALUE self)
{
    return DBL2NUM(SDL_GetWindowBrightness(Get_SDL_Window(self)));
}

/*
 * @overload brightness=(brightness)
 *   Set the brightness (gamma correction) of the window.
 *
 *   @param brightness [Float] the brightness, 0.0 means complete dark and 1.0 means
 *     normal brightness.
 *   @return [brightness]
 *
 *   @see #brightness
 */
static VALUE Window_set_brightness(VALUE self, VALUE brightness)
{
    HANDLE_ERROR(SDL_SetWindowBrightness(Get_SDL_Window(self), NUM2DBL(brightness)));
    return brightness;
}

/*
 * Get the {SDL2::Window::Flags Window flag masks} of the window.
 *
 * @return [Integer] flags
 * @see .create
 */
static VALUE Window_flags(VALUE self)
{
    return UINT2NUM(SDL_GetWindowFlags(Get_SDL_Window(self)));
}

static VALUE gamma_table_to_Array(Uint16 r[])
{
    int i;
    VALUE ary = rb_ary_new2(256);
    for (i=0; i<256; ++i)
        rb_ary_push(ary, UINT2NUM(r[i]));
    return ary;
}

/*
 * Get the gamma ramp for a window
 *
 * @return [Array<Array<Integer>>] the gamma ramp,
 *   return value is red, green, and blue gamma tables and each gamma table
 *   has 256 Integers of 0-65535.
 *
 */
static VALUE Window_gamma_ramp(VALUE self)
{
    Uint16 r[256], g[256], b[256];
    HANDLE_ERROR(SDL_GetWindowGammaRamp(Get_SDL_Window(self), r, g, b));
    return rb_ary_new3(3,
                       gamma_table_to_Array(r),
                       gamma_table_to_Array(g),
                       gamma_table_to_Array(b));
}

/*
 * @overload icon=(icon)
 *
 *   Set the window icon.
 *   
 *   @param icon [SDL2::Surface] the icon for the window
 *   @return [icon]
 */
static VALUE Window_set_icon(VALUE self, VALUE icon)
{
    SDL_SetWindowIcon(Get_SDL_Window(self), Get_SDL_Surface(icon));
    return icon;
}

/*
 *  Return true if the input is grabbed to the window.
 *
 *  @see #input_is_grabbed=
 */
static VALUE Window_input_is_grabbed_p(VALUE self)
{
    return INT2BOOL(SDL_GetWindowGrab(Get_SDL_Window(self)));
}

/*
 * @overload input_is_grabbed=(grabbed) 
 *   Set the window's input grab mode.
 *
 *   @param grabbed [Boolean] true to grub input, and false to release input
 *   @return [grabbed]
 *
 *   @see #input_is_grabbed?
 */
static VALUE Window_set_input_is_grabbed(VALUE self, VALUE grabbed)
{
    SDL_SetWindowGrab(Get_SDL_Window(self), RTEST(grabbed));
    return grabbed;
}

static VALUE Window_get_int_int(void (*func)(SDL_Window*, int*, int*), VALUE window)
{
    int n, m;
    func(Get_SDL_Window(window), &n, &m);
    return rb_ary_new3(2, INT2NUM(n), INT2NUM(m));
}

static VALUE Window_set_int_int(void (*func)(SDL_Window*, int, int), VALUE window, VALUE val)
{
    Check_Type(val, T_ARRAY);
    if (RARRAY_LEN(val) != 2)
        rb_raise(rb_eArgError, "Wrong array size (%ld for 2)", RARRAY_LEN(val));
    func(Get_SDL_Window(window), NUM2INT(rb_ary_entry(val, 0)), NUM2INT(rb_ary_entry(val, 1)));
    return Qnil;
}

/*
 * Get the maximum size of the window's client area.
 *
 * @return [Integer,Integer] maximum width and maximum height.
 *
 * @see #maximum_size=
 */
static VALUE Window_maximum_size(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowMaximumSize, self);
}

/*
 * @overload maximum_size=(size) 
 *   Set the maximum size of the window's client area.
 *
 *   @param size [[Integer, Integer]] maximum width and maximum height,
 *     the both must be positive.
 *
 *   @return [size]
 *
 *   @see #maximum_size
 */
static VALUE Window_set_maximum_size(VALUE self, VALUE max_size)
{
    return Window_set_int_int(SDL_SetWindowMaximumSize, self, max_size);
}

/*
 * Get the minimum size of the window's client area.
 *
 * @return [Integer,Integer] minimum width and minimum height.
 *
 * @see #minimum_size=
 */
static VALUE Window_minimum_size(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowMinimumSize, self);
}

/*
 * @overload minimum_size=(size) 
 *   Set the minimum size of the window's client area.
 *
 *   @param size [[Integer, Integer]] minimum width and minimum height,
 *     the both must be positive.
 *
 *   @return [size]
 *
 *   @see #minimum_size
 */
static VALUE Window_set_minimum_size(VALUE self, VALUE min_size)
{
    return Window_set_int_int(SDL_SetWindowMinimumSize, self, min_size);
}

/*
 * Get the position of the window.
 *
 * @return [Integer,Integer] the x position and the y position
 *
 * @see #position=
 */
static VALUE Window_position(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowPosition, self);
}

/*
 * @overload position=(xy) 
 *   Set the position of the window
 *
 *   @param xy [[Integer, Integer]] the x position and the y position,
 *     {SDL2::Window::POS_CENTERED} and {SDL2::Window::POS_UNDEFINED}
 *     are available.
 *
 *   @return [size]
 *
 *   @see #position
 */
static VALUE Window_set_position(VALUE self, VALUE xy)
{
    return Window_set_int_int(SDL_SetWindowPosition, self, xy);
}

/*
 * Get the size of the window.
 *
 * @return [[Integer, Integer]] the width and the height
 * 
 * @see size=
 */
static VALUE Window_size(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowSize, self);
}

/*
 * @overload size=(size)
 *   Set the size of the window.
 *
 *   @param wh [[Integer, Integer]] new width and new height
 *
 *   @return [size]
 *   
 *   @see #size
 */
static VALUE Window_set_size(VALUE self, VALUE size)
{
    return Window_set_int_int(SDL_SetWindowSize, self, size);
}

/*
 * Get the title of the window
 *
 * @return [String] the title, in UTF-8 encoding
 *
 * @see #title=
 */
static VALUE Window_title(VALUE self)
{
    return utf8str_new_cstr(SDL_GetWindowTitle(Get_SDL_Window(self)));
}

/*
 * Return true if the window is bordered
 *
 * @return [Boolean]
 *
 * @see #bordered=
 */
static VALUE Window_bordered(VALUE self)
{
    return INT2BOOL(!(SDL_GetWindowFlags(Get_SDL_Window(self)) & SDL_WINDOW_BORDERLESS));
}

/*
 * @overload bordered=(bordered) 
 *   Set the border state of the window.
 *
 *   @param bordered [Boolean] true for bordered window, anad false for
 *     borderless window
 *
 *   @return [bordered]
 *
 *   @see #bordered
 */
static VALUE Window_set_bordered(VALUE self, VALUE bordered)
{
    SDL_SetWindowBordered(Get_SDL_Window(self), RTEST(bordered));
    return bordered;
}

/*
 * @overload title=(title)
 *   Set the title of the window.
 *
 *   @param title [String] the title
 *   @return [title]
 *
 *   @see #title
 */
static VALUE Window_set_title(VALUE self, VALUE title)
{
    title = rb_str_export_to_enc(title, rb_utf8_encoding());
    SDL_SetWindowTitle(Get_SDL_Window(self), StringValueCStr(title));
    return Qnil;
}

/*
define(`SIMPLE_WINDOW_METHOD',`static VALUE Window_$2(VALUE self)
{
    SDL_$1Window(Get_SDL_Window(self)); return Qnil;
}')
*/
/*
 * Show the window.
 *
 * @return [nil]
 * @see #hide
 */
SIMPLE_WINDOW_METHOD(Show, show);

/*
 * Hide the window.
 *
 * @return [nil]
 * @see #show
 */
SIMPLE_WINDOW_METHOD(Hide, hide);

/*
 * Maximize the window.
 *
 * @return [nil]
 * @see #minimize
 * @see #restore
 */
SIMPLE_WINDOW_METHOD(Maximize, maximize);

/*
 * Minimize the window.
 *
 * @return [nil]
 * @see #maximize
 * @see #restore
 */
SIMPLE_WINDOW_METHOD(Minimize, minimize);

/*
 * Raise the window above other windows and set the input focus.
 *
 * @return [nil]
 */
SIMPLE_WINDOW_METHOD(Raise, raise);

/*
 * Restore the size and position of a minimized or maixmized window.
 *
 * @return [nil]
 * @see #minimize
 * @see #maximize
 */
SIMPLE_WINDOW_METHOD(Restore, restore);

/*
 * Get the fullscreen stete of the window
 *
 * @return [Integer] 0 for window mode, {SDL2::Window::Flags::FULLSCREEN} for 
 *   fullscreen mode, and {SDL2::Window::Flags::FULLSCREEN_DESKTOP} for fullscreen
 *   at the current desktop resolution.
 *
 * @see #fullscreen_mode=
 * @see #flags
 */
static VALUE Window_fullscreen_mode(VALUE self)
{
    Uint32 flags = SDL_GetWindowFlags(Get_SDL_Window(self));
    return UINT2NUM(flags & (SDL_WINDOW_FULLSCREEN|SDL_WINDOW_FULLSCREEN_DESKTOP));
}

/*
 * @overload fullscreen_mode=(flag)
 *   Set the fullscreen state of the window
 *
 *   @param flag [Integer] 0 for window mode, {SDL2::Window::Flags::FULLSCREEN} for 
 *     fullscreen mode, and {SDL2::Flags::Window::FULLSCREEN_DESKTOP} for fullscreen
 *     at the current desktop resolution.
 *   @return [flag]
 *   
 *   @see #fullscreen_mode
 */
static VALUE Window_set_fullscreen_mode(VALUE self, VALUE flags)
{
    HANDLE_ERROR(SDL_SetWindowFullscreen(Get_SDL_Window(self), NUM2UINT(flags)));
    return flags;
}

#if SDL_VERSION_ATLEAST(2,0,1)
/*
 * Get the size of the drawable region.
 *
 * @return [[Integer, Integer]] the width and height of the region
 */
static VALUE Window_gl_drawable_size(VALUE self)
{
    int w, h;
    SDL_GL_GetDrawableSize(Get_SDL_Window(self), &w, &h);
    return rb_ary_new3(2, INT2NUM(w), INT2NUM(h));
}
#endif

/*
 * Swap the OpenGL buffers for the window, if double buffering
 * is supported.
 *
 * @return [nil]
 */
static VALUE Window_gl_swap(VALUE self)
{
    SDL_GL_SwapWindow(Get_SDL_Window(self));
    return Qnil;
}

/* @return [String] inspection string */
static VALUE Window_inspect(VALUE self)
{
    Window* w = Get_Window(self);
    if (w->window)
        return rb_sprintf("<%s:%p window_id=%d>",
                          rb_obj_classname(self), (void*)self, SDL_GetWindowID(w->window));
    else
        return rb_sprintf("<%s:%p (destroyed)>", rb_obj_classname(self), (void*)self);
}

/* @return [Hash] (GC) debug information */
static VALUE Window_debug_info(VALUE self)
{
    Window* w = Get_Window(self);
    VALUE info = rb_hash_new();
    int num_active_renderers = 0;
    int i;
    rb_hash_aset(info, rb_str_new2("destroy?"), INT2BOOL(w->window == NULL));
    rb_hash_aset(info, rb_str_new2("max_renderers"), INT2NUM(w->max_renderers));
    rb_hash_aset(info, rb_str_new2("num_renderers"), INT2NUM(w->num_renderers));
    for (i=0; i<w->num_renderers; ++i)
        if (w->renderers[i]->renderer)
            ++num_active_renderers;
    rb_hash_aset(info, rb_str_new2("num_active_renderers"), INT2NUM(num_active_renderers));
  
    return info;
}

/*
 * Document-module: SDL2::Window::Flags
 *
 * OR'd bits of the constants of this module represents window states.
 * 
 * You can see a window state using {SDL2::Window#flags}
 * and create a window with a specified
 * state using flag parameter of {SDL2::Window.create}.
 *
 */

/*
 * Document-class: SDL2::Display
 *
 * This class represents displays, screens, or monitors.
 *
 * This means that if you use dual screen, {.displays} returns two displays.
 *
 * This class handles color depth, resolution, and refresh rate of displays.
 *
 *
 * @!attribute [r] index
 *   The index of the display, 0 origin
 *   @return [Integer]
 *
 * @!attribute [r] name
 *   The name of the display
 *   @return [Stirng]
 *
 */

/*
 * Get all connected displays.
 *
 * @return [Array<SDL2::Display>]
 * 
 */
static VALUE Display_s_displays(VALUE self)
{
    int i;
    int num_displays = HANDLE_ERROR(SDL_GetNumVideoDisplays());
    VALUE displays = rb_ary_new2(num_displays);
    for (i=0; i<num_displays; ++i)
        rb_ary_push(displays, Display_new(i));
    return displays;
}

static int Display_index_int(VALUE display)
{
    return NUM2INT(rb_iv_get(display, "@index"));
}

/*
 * Get available display modes of the display.
 *
 * @return [Array<SDL2::Display::Mode>]
 * 
 */
static VALUE Display_modes(VALUE self)
{
    int i;
    int index = Display_index_int(self);
    int num_modes = SDL_GetNumDisplayModes(index);
    VALUE modes = rb_ary_new2(num_modes);
    for (i=0; i<num_modes; ++i) {
        SDL_DisplayMode mode;
        HANDLE_ERROR(SDL_GetDisplayMode(index, i, &mode));
        rb_ary_push(modes, DisplayMode_new(&mode));
    }
    return modes;
}

/*
 * Get the current display mode.
 *
 * @return [SDL2::Display::Mode]
 * 
 * @see #desktop_mode
 */
static VALUE Display_current_mode(VALUE self)
{
    SDL_DisplayMode mode;
    HANDLE_ERROR(SDL_GetCurrentDisplayMode(Display_index_int(self), &mode));
    return DisplayMode_new(&mode);
}

/*
 * Get the desktop display mode.
 *
 * Normally, the return value of this method is
 * same as {#current_mode}. However, 
 * when you use fullscreen and chagne the resolution,
 * this method returns the previous native display mode,
 * and not the current mode.
 *
 * @return [SDL2::Display::Mode]
 */
static VALUE Display_desktop_mode(VALUE self)
{
    SDL_DisplayMode mode;
    HANDLE_ERROR(SDL_GetDesktopDisplayMode(Display_index_int(self), &mode));
    return DisplayMode_new(&mode);
}

/*
 * @overload closest_mode(mode) 
 *   Get the available display mode closest match to **mode**.
 *   
 *   @param mode [SDL2::Display::Mode] the desired display mode
 *   @return [SDL2::Display::Mode]
 *   
 */
static VALUE Display_closest_mode(VALUE self, VALUE mode)
{
    SDL_DisplayMode closest;
    if (!SDL_GetClosestDisplayMode(Display_index_int(self), Get_SDL_DisplayMode(mode),
                                   &closest))
        SDL_ERROR();
    return DisplayMode_new(&closest);
}

/*
 * Get the desktop area represented by the display, with the primary
 * display located at (0, 0).
 *
 * @return [Rect]
 */
static VALUE Display_bounds(VALUE self)
{
    VALUE rect = rb_obj_alloc(cRect);
    HANDLE_ERROR(SDL_GetDisplayBounds(Display_index_int(self), Get_SDL_Rect(rect)));
    return rect;
}

static Uint32 uint32_for_format(VALUE format)
{
    if (rb_obj_is_kind_of(format, cPixelFormat))
        return NUM2UINT(rb_iv_get(format, "@format"));
    else
        return NUM2UINT(format);
}

/*
 * Document-class: SDL2::Display::Mode
 *
 * This class represents the display mode.
 *
 * An object of this class has information about color depth, refresh rate,
 * and resolution of a display.
 *
 *
 */

/*
 * @overload initialze(format, w, h, refresh_rate)
 *   Create a new Display::Mode object.
 *
 *   @param format [SDL2::PixelFormat, Integer] pixel format
 *   @param w [Integer] the width
 *   @param h [Integer] the height
 *   @param refresh_rate [Integer] refresh rate
 */
static VALUE DisplayMode_initialize(VALUE self, VALUE format, VALUE w, VALUE h,
                                    VALUE refresh_rate)
{
    SDL_DisplayMode* mode = Get_SDL_DisplayMode(self);
    mode->format = uint32_for_format(format);
    mode->w = NUM2INT(w); mode->h = NUM2INT(h);
    mode->refresh_rate = NUM2INT(refresh_rate);
    return Qnil;
}

/* @return [String] inspection string */
static VALUE DisplayMode_inspect(VALUE self)
{
    SDL_DisplayMode* mode = Get_SDL_DisplayMode(self);
    return rb_sprintf("<%s: format=%s w=%d h=%d refresh_rate=%d>",
                      rb_obj_classname(self), SDL_GetPixelFormatName(mode->format),
                      mode->w, mode->h, mode->refresh_rate);
                      
}

/* @return [SDL2::PixelFormat] the pixel format of the display mode */
static VALUE DisplayMode_format(VALUE self)
{
    return PixelFormat_new(Get_SDL_DisplayMode(self)->format);
}

/* @return [Integer] the width of the screen of the display mode */
static VALUE DisplayMode_w(VALUE self)
{
    return INT2NUM(Get_SDL_DisplayMode(self)->w);
}

/* @return [Integer] the height of the screen of the display mode */
static VALUE DisplayMode_h(VALUE self)
{
    return INT2NUM(Get_SDL_DisplayMode(self)->h);
}

/* @return [Integer] the refresh rate of the display mode */
static VALUE DisplayMode_refresh_rate(VALUE self)
{
    return INT2NUM(Get_SDL_DisplayMode(self)->refresh_rate);
}

/*
 * Document-class: SDL2::Renderer
 *
 * This class represents a 2D rendering context for a window.
 *
 * You can create a renderer using {SDL2::Window#create_renderer} and
 * use it to draw figures on the window.
 *      
 *
 * @!method destroy?
 *   Return true if the renderer is {#destroy destroyed}.
 *
 */


/*
 * @overload drivers_info
 *   Return information of all available rendering contexts.
 *   @return [Array<SDL2::Renderer::Info>] information about rendering contexts
 *   
 */
static VALUE Renderer_s_drivers_info(VALUE self)
{
    int num_drivers = SDL_GetNumRenderDrivers();
    VALUE info_ary = rb_ary_new();
    int i;
    for (i=0; i<num_drivers; ++i) {
        SDL_RendererInfo info;
        HANDLE_ERROR(SDL_GetRenderDriverInfo(i, &info));
        rb_ary_push(info_ary, RendererInfo_new(&info));
    }
    return info_ary;
}

/*
 * Destroy the rendering context and free associated textures.
 *
 * @return [nil]
 * @see #destroy?
 */
static VALUE Renderer_destroy(VALUE self)
{
    Renderer_destroy_internal(Get_Renderer(self));
    return Qnil;
}

/*
 * @overload create_texture(format, access, w, h)
 *   Create a new texture for the rendering context.
 *
 *   You can use the following constants to specify access pattern
 *   
 *   * {SDL2::Texture::ACCESS_STATIC}
 *   * {SDL2::Texture::ACCESS_STREAMING}
 *   * {SDL2::Texture::ACCESS_TARGET}
 *     
 *   @param [SDL2::PixelFormat,Integer] format format of the texture
 *   @param [Integer] access texture access pattern
 *   @param [Integer] w the width ofthe texture in pixels
 *   @param [Integer] h the height ofthe texture in pixels
 *   
 *   @return [SDL2::Texture] the created texture
 *
 *   @raise [SDL2::Error] raised when the texture cannot be created
 *
 *   @see #create_texture_from
 */
static VALUE Renderer_create_texture(VALUE self, VALUE format, VALUE access,
                                     VALUE w, VALUE h)
{
    SDL_Texture* texture = SDL_CreateTexture(Get_SDL_Renderer(self),
                                             uint32_for_format(format),
                                             NUM2INT(access), NUM2INT(w), NUM2INT(h));
    if (!texture)
        SDL_ERROR();
    return Texture_new(texture, Get_Renderer(self));
}

/*
 * @overload create_texture_from(surface)
 *   Create a texture from an existing surface.
 *
 *   @param [SDL2::Surface] surface the surface containing pixels for the texture
 *   @return [SDL2::Texture] the created texture
 *
 *   @raise [SDL2::Error] raised when the texture cannot be created
 *
 *   @see #create_texture
 */
static VALUE Renderer_create_texture_from(VALUE self, VALUE surface)
{
    SDL_Texture* texture = SDL_CreateTextureFromSurface(Get_SDL_Renderer(self),
                                                        Get_SDL_Surface(surface));
    if (texture == NULL)
        SDL_ERROR();
  
    return Texture_new(texture, Get_Renderer(self));
}

static SDL_Rect* Get_SDL_Rect_or_NULL(VALUE rect)
{
    return rect == Qnil ? NULL : Get_SDL_Rect(rect);
}

static SDL_Point* Get_SDL_Point_or_NULL(VALUE point)
{
    return point == Qnil ? NULL : Get_SDL_Point(point);
}

/*
 * @overload copy(texture, srcrect, dstrect)
 *   Copy a portion of the texture to the current rendering target.
 *
 *   @param [SDL2::Texture] texture the source texture
 *   @param [SDL2::Rect,nil] srcrect the source rectangle, or nil for the entire texture
 *   @param [SDL2::Rect,nil] dstrect the destination rectangle, or nil for the entire
 *     rendering target; the texture will be stretched to fill the given rectangle
 *
 *   @return [void]
 *   
 *   @see #copy_ex
 */
static VALUE Renderer_copy(VALUE self, VALUE texture, VALUE srcrect, VALUE dstrect)
{
    HANDLE_ERROR(SDL_RenderCopy(Get_SDL_Renderer(self),
                                Get_SDL_Texture(texture),
                                Get_SDL_Rect_or_NULL(srcrect),
                                Get_SDL_Rect_or_NULL(dstrect)));
    return Qnil;
}

/*
 * @overload copy_ex(texture, srcrect, dstrect, angle, center, flip)
 *   Copy a portion of the texture to the current rendering target,
 *   rotating it by angle around the given center and also flipping
 *   it top-bottom and/or left-right.
 *
 *   You can use the following constants to specify the horizontal/vertical flip:
 *   
 *   * {SDL2::Renderer::FLIP_HORIZONTAL} - flip horizontally
 *   * {SDL2::Renderer::FLIP_VERTICAL} - flip vertically
 *   * {SDL2::Renderer::FLIP_NONE} - do not flip, equal to zero
 *
 *   
 *   @param [SDL2::Texture] texture the source texture
 *   @param [SDL2::Rect,nil] srcrect the source rectangle, or nil for the entire texture
 *   @param [SDL2::Rect,nil] dstrect the destination rectangle, or nil for the entire
 *     rendering target; the texture will be stretched to fill the given rectangle
 *   @param [Float] angle an angle in degree indicating the rotation
 *     that will be applied to dstrect
 *   @param [SDL2::Point,nil] center the point around which dstrect will be rotated,
 *     (if nil, rotation will be done around the center of dstrect)
 *   @param [Integer] flip bits OR'd of the flip consntants
 *
 *   @return [void]
 *
 *   @see #copy
 */
static VALUE Renderer_copy_ex(VALUE self, VALUE texture, VALUE srcrect, VALUE dstrect,
                              VALUE angle, VALUE center, VALUE flip)
{
    HANDLE_ERROR(SDL_RenderCopyEx(Get_SDL_Renderer(self),
                                  Get_SDL_Texture(texture),
                                  Get_SDL_Rect_or_NULL(srcrect),
                                  Get_SDL_Rect_or_NULL(dstrect),
                                  NUM2DBL(angle),
                                  Get_SDL_Point_or_NULL(center),
                                  NUM2INT(flip)));
    return Qnil;
}

/*
 * Update the screen with rendering performed
 * @return [nil]
 */
static VALUE Renderer_present(VALUE self)
{
    SDL_RenderPresent(Get_SDL_Renderer(self));
    return Qnil;
}

/*
 * Crear the rendering target with the drawing color.
 * @return [nil]
 * 
 * @see #draw_color=
 */
static VALUE Renderer_clear(VALUE self)
{
    HANDLE_ERROR(SDL_RenderClear(Get_SDL_Renderer(self)));
    return Qnil;
}

/* 
 * Get the color used for drawing operations
 * @return [[Integer,Integer,Integer,Integer]]
 *   red, green, blue, and alpha components of the drawing color
 *   (all components are more than or equal to 0 and less than and equal to 255)
 * 
 * @see #draw_color=
 */
static VALUE Renderer_draw_color(VALUE self)
{
    Uint8 r, g, b, a;
    HANDLE_ERROR(SDL_GetRenderDrawColor(Get_SDL_Renderer(self), &r, &g, &b, &a));
    return rb_ary_new3(4, INT2FIX(r), INT2FIX(g), INT2FIX(b), INT2FIX(a));
}

/*
 * @overload draw_color=(color)
 *   Set the color used for drawing operations
 *
 *   All color components (including alpha) must be more than or equal to 0
 *   and less than and equal to 255
 *
 *   This method effects the following methods.
 *   
 *   * {#draw_line}
 *   * {#draw_point}
 *   * {#draw_rect}
 *   * {#fill_rect}
 *   * {#clear}
 *   
 *   @param [[Integer, Integer, Integer]] color
 *     red, green, and blue components used for drawing
 *   @param [[Integer, Integer, Integer, Integer]] color
 *     red, green, blue, and alpha components used for drawing
 *
 *   @return [color]
 *   
 *   @see #draw_color
 */
static VALUE Renderer_set_draw_color(VALUE self, VALUE rgba)
{
    SDL_Color color = Array_to_SDL_Color(rgba);
    
    HANDLE_ERROR(SDL_SetRenderDrawColor(Get_SDL_Renderer(self),
                                        color.r, color.g, color.b, color.a));
                                        
    return rgba;
}

/*
 * @overload draw_line(x1, y1, x2, y2)
 *   Draw a line from (x1, y1) to (x2, y2) using drawing color given by
 *   {#draw_color=}.
 *
 *   @param [Integer] x1 the x coordinate of the start point
 *   @param [Integer] y1 the y coordinate of the start point
 *   @param [Integer] x2 the x coordinate of the end point
 *   @param [Integer] y2 the y coordinate of the end point
 *   @return [nil]
 */
static VALUE Renderer_draw_line(VALUE self, VALUE x1, VALUE y1, VALUE x2, VALUE y2)
{
    HANDLE_ERROR(SDL_RenderDrawLine(Get_SDL_Renderer(self),
                                    NUM2INT(x1), NUM2INT(y1), NUM2INT(x2), NUM2INT(y2)));
    return Qnil;
}

/*
 * @overload draw_point(x, y)
 *   Draw a point at (x, y) using drawing color given by {#draw_color=}.
 *
 *   @param [Integer] x the x coordinate of the point
 *   @param [Integer] y the y coordinate of the point
 *
 *   @return [nil]
 */
static VALUE Renderer_draw_point(VALUE self, VALUE x, VALUE y)
{
    HANDLE_ERROR(SDL_RenderDrawPoint(Get_SDL_Renderer(self), NUM2INT(x), NUM2INT(y)));
    return Qnil;
}

/*
 * @overload draw_rect(rect)
 *   Draw a rectangle using drawing color given by {#draw_color=}.
 *
 *   @param [SDL2::Rect] rect the drawing rectangle
 *   
 *   @return [nil]
 */
static VALUE Renderer_draw_rect(VALUE self, VALUE rect)
{
    HANDLE_ERROR(SDL_RenderDrawRect(Get_SDL_Renderer(self), Get_SDL_Rect(rect)));
    return Qnil;
}

/*
 * @overload fill_rect(rect)
 *   Draw a filled rectangle using drawing color given by {#draw_color=}.
 *
 *   @param [SDL2::Rect] rect the drawing rectangle
 *   
 *   @return [nil]
 */
static VALUE Renderer_fill_rect(VALUE self, VALUE rect)
{
    HANDLE_ERROR(SDL_RenderFillRect(Get_SDL_Renderer(self), Get_SDL_Rect(rect)));
    return Qnil;
}

/*
 * Get information about _self_ rendering context .
 *
 * @return [SDL2::Renderer::Info] rendering information
 */
static VALUE Renderer_info(VALUE self)
{
    SDL_RendererInfo info;
    HANDLE_ERROR(SDL_GetRendererInfo(Get_SDL_Renderer(self), &info));
    return RendererInfo_new(&info);
}

/*
 * Get the blend mode used for drawing operations like
 * {#fill_rect} and {#draw_line}.
 *
 * @return [Integer]
 * 
 * @see #draw_blend_mode=
 * @see SDL2::BlendMode
 */
static VALUE Renderer_draw_blend_mode(VALUE self)
{
    SDL_BlendMode mode;
    HANDLE_ERROR(SDL_GetRenderDrawBlendMode(Get_SDL_Renderer(self), &mode));
    return INT2FIX(mode);
}

/*
 * @overload draw_blend_mode=(mode)
 *   Set the blend mode used for drawing operations.
 *
 *   This method effects the following methods.
 *   
 *   * {#draw_line}
 *   * {#draw_point}
 *   * {#draw_rect}
 *   * {#fill_rect}
 *   * {#clear}
 *
 *   @param mode [Integer] the blending mode
 *   @return mode
 *
 *   @see #draw_blend_mode
 *   @see SDL2::BlendMode
 */
static VALUE Renderer_set_draw_blend_mode(VALUE self, VALUE mode)
{
    HANDLE_ERROR(SDL_SetRenderDrawBlendMode(Get_SDL_Renderer(self), NUM2INT(mode)));
    return mode;
}

/*
 * Get the clip rectangle for the current target.
 *
 * @return [SDL2::Rect] the current clip rectangle
 * @see {#clip_rect=}
 */
static VALUE Renderer_clip_rect(VALUE self)
{
    VALUE rect = rb_obj_alloc(cRect);
    SDL_RenderGetClipRect(Get_SDL_Renderer(self), Get_SDL_Rect(rect));
    return rect;
}

/*
 * @overload clip_rect=(rect)
 *
 *   Set the clip rectangle for the current target.
 *
 *   @return [rect]
 *   @see #clip_rect
 */
static VALUE Renderer_set_clip_rect(VALUE self, VALUE rect)
{
    HANDLE_ERROR(SDL_RenderSetClipRect(Get_SDL_Renderer(self), Get_SDL_Rect(rect)));
    return rect;
}

#if SDL_VERSION_ATLEAST(2,0,4)
/*
 * Get whether clipping is enabled on the renderer.
 *
 * @note This method is available since SDL 2.0.4.
 */
static VALUE Render_clip_enabled_p(VALUE self)
{
    return INT2BOOL(SDL_RenderIsClipEnabled(Get_SDL_Renderer(self)));
}
#endif

/*
 * Get device indepndent resolution for rendering.
 *
 * @return [[Integer, Integer]] the logical width and height
 * @see #logical_size=
 */
static VALUE Renderer_logical_size(VALUE self)
{
    int w, h;
    SDL_RenderGetLogicalSize(Get_SDL_Renderer(self), &w, &h);
    return rb_ary_new3(2, INT2FIX(w), INT2FIX(h));
}

/*
 * @overload logical_size=(w_and_h)
 *
 *   Set a device indepndent resolution for rendering.
 *   
 *   @param w_and_h [[Integer, Integer]] the width and height of the logical resolution
 *   @return [w_and_h]
 *   @see #logical_size
 */
static VALUE Renderer_set_logical_size(VALUE self, VALUE wh)
{
    HANDLE_ERROR(SDL_RenderSetLogicalSize(Get_SDL_Renderer(self),
                                          NUM2DBL(rb_ary_entry(wh, 0)),
                                          NUM2DBL(rb_ary_entry(wh, 1))));
    return wh;
}

/*
 * Get the drawing scale for the current target.
 *
 * @return [[Integer, Integer]] horizontal and vertical scale factor
 * @see #scale=
 */
static VALUE Renderer_scale(VALUE self)
{
    float scaleX, scaleY;
    SDL_RenderGetScale(Get_SDL_Renderer(self), &scaleX, &scaleY);
    return rb_ary_new3(2, DBL2NUM(scaleX), DBL2NUM(scaleY));
}

/*
 * @overload scale=(scaleX_and_scaleY)
 *
 *   Set the drawing scale for rendering.
 *
 *   The drawing coordinates are scaled by the x/y scaling factors before they are used by the renderer.
 *   This allows resolution independent drawing with a single coordinate system.
 *   
 *   If this results in scaling or subpixel drawing by the rendering backend,
 *   it will be handled using the appropriate
 *   quality hints. For best results use integer scaling factors.
 *
 *   @param scaleX_and_scaleY [[Float, Float]] the horizontal and vertical scaling factors
 *   @return [scaleX_and_scaleY]
 *   @see #scale
 */
static VALUE Renderer_set_scale(VALUE self, VALUE xy)
{
    float scaleX, scaleY;
    scaleX = NUM2DBL(rb_ary_entry(xy, 0));
    scaleY = NUM2DBL(rb_ary_entry(xy, 1));
    HANDLE_ERROR(SDL_RenderSetScale(Get_SDL_Renderer(self), scaleX, scaleY));
    return xy;
}

/*
 * Get the drawing area for the current target.
 *
 * @return [SDL2::Rect] the current drawing area
 * @see #viewport=
 */
static VALUE Renderer_viewport(VALUE self)
{
    VALUE rect = rb_obj_alloc(cRect);
    SDL_RenderGetViewport(Get_SDL_Renderer(self), Get_SDL_Rect(rect));
    return rect;
}

/*
 * @overload viewport=(area) 
 *   Set the drawing area for rendering on the current target.
 *
 *   @param area [SDL2::Rect,nil] the drawing area, or nil to set the viewport to the entire target
 *   @return [area]
 *   @see #viewport
 */
static VALUE Renderer_set_viewport(VALUE self, VALUE rect)
{
    HANDLE_ERROR(SDL_RenderSetClipRect(Get_SDL_Renderer(self), Get_SDL_Rect_or_NULL(rect)));
    return rect;
}

/*
 * Return true if the renderer supports render target.
 *
 * @see #render_target=
 */
static VALUE Renderer_support_render_target_p(VALUE self)
{
    return INT2BOOL(SDL_RenderTargetSupported(Get_SDL_Renderer(self)));
}

/*
 * Get the output size of a rendering context.
 *
 * @return [[Integer, Integer]] the width and the height
 */
static VALUE Renderer_output_size(VALUE self)
{
    int w, h;
    HANDLE_ERROR(SDL_GetRendererOutputSize(Get_SDL_Renderer(self), &w, &h));
    return rb_ary_new3(2, INT2FIX(w), INT2FIX(h));
}

/*
 * @overload render_target=(target)
 *  Set a texture as the current render target.
 *  
 *  Some renderers have ability to render to a texture instead of a screen.
 *  You can judge whether your renderer has this ability using
 *  {#support_render_target?}.
 *  
 *  The target texture musbe be {#create_texture created} with the
 *  {SDL2::Texture::ACCESS_TARGET} flag.
 *  
 *  @param [SDL2::Texture,nil] target the targeted texture, or nil
 *    for the default render target(i.e. screen)
 *
 *  @return [target]
 *  
 *  @see #render_target
 */
static VALUE Renderer_set_render_target(VALUE self, VALUE target)
{
    HANDLE_ERROR(SDL_SetRenderTarget(Get_SDL_Renderer(self),
                                     (target == Qnil) ? NULL : Get_SDL_Texture(target)));
    rb_iv_set(self, "render_target", target);
    return target;
}

/*
 * Get the current render target.
 *
 * @return [SDL2::Texture] the current rendering target
 * @return [nil] for the default render target (i.e. screen)
 *
 * @see #render_target=
 * @see #support_render_target?
 */
static VALUE Renderer_render_target(VALUE self)
{
    return rb_iv_get(self, "render_target");
}

/*
 * Reset the render target to the screen.
 *
 * @return [nil]
 */
static VALUE Renderer_reset_render_target(VALUE self)
{
    return Renderer_set_render_target(self, Qnil);
}

/* @return [Hash<String=>Object>] (GC) debug information */
static VALUE Renderer_debug_info(VALUE self)
{
    Renderer* r = Get_Renderer(self);
    VALUE info = rb_hash_new();
    int num_active_textures = 0;
    int i;
    rb_hash_aset(info, rb_str_new2("destroy?"), INT2BOOL(r->renderer == NULL));
    rb_hash_aset(info, rb_str_new2("max_textures"), INT2NUM(r->max_textures));
    rb_hash_aset(info, rb_str_new2("num_textures"), INT2NUM(r->num_textures));
    for (i=0; i<r->num_textures; ++i)
        if (r->textures[i]->texture)
            ++num_active_textures;
    rb_hash_aset(info, rb_str_new2("num_active_textures"), INT2NUM(num_active_textures));
    rb_hash_aset(info, rb_str_new2("refcount"), INT2NUM(r->refcount));
  
    return info;
}

/*
 * Document-class: SDL2::Renderer::Info
 *
 * This class contains information of a rendering context.
 *
 * @!attribute [r] name
 *   @return [String] the name of the renderer
 *
 * @!attribute [r] texture_formats
 *   @return [Array<SDL2::PixelFormat>] available texture formats
 *
 * @!attribute [r] max_texture_width
 *   @return [Integer] maximum texture width
 *   
 * @!attribute [r] max_texture_height
 *   @return [Integer] maximum texture height
 */

/*
 * Document-module: SDL2::Renderer::Flags
 *
 * The OR'd bits of the constants of this module represents
 * the state of renderers.
 * 
 * You can use this flag
 * {SDL2::Window#create_renderer when you create a new renderer}.
 * No flags(==0) gives priority to available ACCELERATED renderers.
 */

/*
 * Document-module: SDL2::BlendMode
 *
 * Constants represent blend mode for {SDL2::Renderer.copy} and
 * drawing operations.
 *
 * You can change the blending mode using
 * {SDL2::Renderer#draw_blend_mode=} and {SDL2::Texture#blend_mode=}.
 *
 * In the backend of SDL, opengl or direct3d blending
 * mechanism is used for blending operations.
 */

/*
 * Document-class: SDL2::Texture
 *
 * This class represents the texture associated with a renderer.
 *
 *
 * @!method destroy?
 *   Return true if the texture is {#destroy destroyed}.
 */

/*
 * Destroy the texture and deallocate memory.
 *
 * @see #destroy?
 */
static VALUE Texture_destroy(VALUE self)
{
    Texture_destroy_internal(Get_Texture(self));
    return Qnil;
}

/*
 * Get the blending mode of the texture.
 *
 * @return [Integer] blend mode
 * 
 * @see #blend_mode=
 */
static VALUE Texture_blend_mode(VALUE self)
{
    SDL_BlendMode mode;
    HANDLE_ERROR(SDL_GetTextureBlendMode(Get_SDL_Texture(self), &mode));
    return INT2FIX(mode);
}

/*
 * @overload blend_mode=(mode)
 *   Set the blending model of the texture.
 *
 *   @param mode [Integer] blending mode
 *   @return [mode]
 *
 *   @see #blend_mode
 */
static VALUE Texture_set_blend_mode(VALUE self, VALUE mode)
{
    HANDLE_ERROR(SDL_SetTextureBlendMode(Get_SDL_Texture(self), NUM2INT(mode)));
    return mode;
}

/*
 * Get an additional alpha value used in render copy operations.
 *
 * @return [Integer] the current alpha value
 *
 * @see #alpha_mod=
 */
static VALUE Texture_alpha_mod(VALUE self)
{
    Uint8 alpha;
    HANDLE_ERROR(SDL_GetTextureAlphaMod(Get_SDL_Texture(self), &alpha));
    return INT2FIX(alpha);
}

/*
 * @overload alpha_mod=(alpha) 
 *   Set an additional alpha value used in render copy operations.
 *
 *   @param alpha [Integer] the alpha value multiplied into copy operation,
 *     from 0 to 255
 *   @return [alpha]
 *
 *   @see #alpha_mod
 */
static VALUE Texture_set_alpha_mod(VALUE self, VALUE alpha)
{
    HANDLE_ERROR(SDL_SetTextureAlphaMod(Get_SDL_Texture(self), NUM2UCHAR(alpha)));
    return alpha;
}

/*
 * Get an additional color value used in render copy operations.
 *
 * @return [[Integer, Integer, Integer]] the current red, green, and blue
 *   color value.
 */
static VALUE Texture_color_mod(VALUE self)
{
    Uint8 r, g, b;
    HANDLE_ERROR(SDL_GetTextureColorMod(Get_SDL_Texture(self), &r, &g, &b));
    return rb_ary_new3(3, INT2FIX(r), INT2FIX(g), INT2FIX(b));
}

/*
 * @overload color_mod=(rgb) 
 *   Set an additional color value used in render copy operations.
 *   
 *   @param rgb [[Integer, Integer, Integer]] the red, green, and blue
 *     color value multiplied into copy operations.
 *   @return [rgb]
 */
static VALUE Texture_set_color_mod(VALUE self, VALUE rgb)
{
    SDL_Color color = Array_to_SDL_Color(rgb);
    HANDLE_ERROR(SDL_SetTextureColorMod(Get_SDL_Texture(self),
                                        color.r, color.g, color.b));
    return rgb;
}

/*
 * Get the format of the texture.
 *
 * @return [SDL2::PixelFormat]
 */
static VALUE Texture_format(VALUE self)
{
    Uint32 format;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), &format, NULL, NULL, NULL));
    return PixelFormat_new(format);
}

/*
 * Get the access pattern allowed for the texture.
 *
 * The return value is one of the following:
 * 
 * * {SDL2::Texture::ACCESS_STATIC}
 * * {SDL2::Texture::ACCESS_STREAMING}
 * * {SDL2::Texture::ACCESS_TARGET}
 *
 * @return [Integer]
 * 
 * @see SDL2::Renderer#create_texture
 */
static VALUE Texture_access_pattern(VALUE self)
{
    int access;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), NULL, &access, NULL, NULL));
    return INT2FIX(access);
}

/*
 * Get the width of the texture.
 *
 * @return [Integer]
 * 
 * @see SDL2::Renderer#create_texture
 */
static VALUE Texture_w(VALUE self)
{
    int w;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), NULL, NULL, &w, NULL));
    return INT2FIX(w);
}

/*
 * Get the height of the texture.
 *
 * @return [Integer]
 * 
 * @see SDL2::Renderer#create_texture
 */
static VALUE Texture_h(VALUE self)
{
    int h;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), NULL, NULL, NULL, &h));
    return INT2FIX(h);
}

/* @return [String] inspection string */
static VALUE Texture_inspect(VALUE self)
{
    Texture* t = Get_Texture(self);
    Uint32 format;
    int access, w, h;
    if (!t->texture)
        return rb_sprintf("<%s: (destroyed)>", rb_obj_classname(self));
    
    HANDLE_ERROR(SDL_QueryTexture(t->texture, &format, &access, &w, &h));
    return rb_sprintf("<%s:%p format=%s access=%d w=%d h=%d>",
                      rb_obj_classname(self), (void*)self, SDL_GetPixelFormatName(format),
                      access, w, h);
}

/* @return [Hash<String=>Object>] (GC) debugging information */
static VALUE Texture_debug_info(VALUE self)
{
    Texture* t = Get_Texture(self);
    VALUE info = rb_hash_new();
    rb_hash_aset(info, rb_str_new2("destroy?"), INT2BOOL(t->texture == NULL));
    rb_hash_aset(info, rb_str_new2("refcount"), INT2NUM(t->refcount));
    return info;
}

/*
 * Document-class: SDL2::Surface
 *
 * This class represents bitmap images (collection of pixels).
 *
 * Normally in SDL2, this class is not used for drawing a image
 * on a window. {SDL2::Texture} is used for the purpose.
 *
 * Mainly this class is for compatibility with SDL1, but the class
 * is useful for simple pixel manipulation.
 * For example, {SDL2::TTF} can create only surfaces, not textures.
 * You can convert a surface to texture with
 * {SDL2::Renderer#create_texture_from}.
 *
 * @!method destroy?
 *   Return true if the surface is {#destroy destroyed}.
 *
 */

/*
 * @overload load_bmp(path)
 *   Load a surface from bmp file.
 *
 *   @param path [String] bmp file path
 *   @return [SDL2::Surface]
 *   @raise [SDL2::Error] raised when an error occurs.
 *     For example, if there is no file or the file file
 *     format is not Windows BMP.
 *
 */
static VALUE Surface_s_load_bmp(VALUE self, VALUE fname)
{
    SDL_Surface* surface = SDL_LoadBMP(StringValueCStr(fname));

    if (surface == NULL)
        HANDLE_ERROR(-1);

    return Surface_new(surface);
}

/*
 * @overload from_string(string, width, heigth, depth, pitch=nil, rmask=nil, gmask=nil, bmask=nil, amask=nil)
 *
 *   Create a RGB surface from pixel data as String object.
 *
 *   If rmask, gmask, bmask are omitted, the default masks are used.
 *   If amask is omitted, alpha mask is considered to be zero.
 *   
 *   @param string [String] the pixel data
 *   @param width [Integer] the width of the creating surface
 *   @param height [Integer] the height of the creating surface
 *   @param depth [Integer] the color depth (in bits) of the creating surface
 *   @param pitch [Integer] the number of bytes of one scanline
 *     if this argument is omitted, width*depth/8 is used.
 *   @param rmask [Integer] the red mask of a pixel
 *   @param gmask [Integer] the green mask of a pixel
 *   @param bmask [Integer] the blue mask of a pixel
 *   @param amask [Integer] the alpha mask of a pixel
 *   @return [SDL2::Surface] a new surface
 *   @raise [SDL2::Error] raised when an error occurs in C SDL library
 *   
 */
static VALUE Surface_s_from_string(int argc, VALUE* argv, VALUE self)
{
    VALUE string, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask;
    int w, h, d, p, r, g, b, a;
    SDL_Surface* surface;
    void* pixels;
    Surface* s;
    
    rb_scan_args(argc, argv, "45", &string, &width, &height, &depth,
                 &pitch, &Rmask, &Gmask, &Bmask, &Amask);
    StringValue(string);
    w = NUM2INT(width);
    h = NUM2INT(height);
    d = NUM2INT(depth);
    p = (pitch == Qnil) ? d*w/8 : NUM2INT(pitch);
    r = (Rmask == Qnil) ? 0 : NUM2UINT(Rmask);
    g = (Gmask == Qnil) ? 0 : NUM2UINT(Gmask);
    b = (Bmask == Qnil) ? 0 : NUM2UINT(Bmask);
    a = (Amask == Qnil) ? 0 : NUM2UINT(Amask);

    if (RSTRING_LEN(string) < p*h)
        rb_raise(rb_eArgError, "String too short");
    if (p < d*w/8 )
        rb_raise(rb_eArgError, "pitch too small");

    pixels = ruby_xmalloc(RSTRING_LEN(string));
    memcpy(pixels, RSTRING_PTR(string), RSTRING_LEN(string));
    surface = SDL_CreateRGBSurfaceFrom(pixels, w, h, d, p, r, g, b, a);
    if (!surface)
        SDL_ERROR();
    
    RB_GC_GUARD(string);

    s = ALLOC(Surface);
    s->surface = surface;
    s->need_to_free_pixels = 1;
    return Data_Wrap_Struct(cSurface, 0, Surface_free, s);
}

/*
 * Destroy the surface and deallocate the memory for pixels.
 *
 * @return [nil]
 * @see #destroy?
 */
static VALUE Surface_destroy(VALUE self)
{
    Surface* s = Get_Surface(self);
    if (s->need_to_free_pixels)
        free(s->surface->pixels);
    s->need_to_free_pixels = 0;
    if (s->surface)
        SDL_FreeSurface(s->surface);
    s->surface = NULL;
    return Qnil;
}

/*
 * Get the blending mode of the surface used for blit operations.
 *
 * @return [Integer]
 * @see #blend_mode=
 */
static VALUE Surface_blend_mode(VALUE self)
{
    SDL_BlendMode mode;
    HANDLE_ERROR(SDL_GetSurfaceBlendMode(Get_SDL_Surface(self), &mode));
    return INT2FIX(mode);
}

/*
 * @overload blend_mode=(mode)
 *   Set the blending mode of the surface used for blit operations.
 *
 *   @param mode [Integer] the blending mode
 *   @return [mode]
 *   @see #blend_mode
 */
static VALUE Surface_set_blend_mode(VALUE self, VALUE mode)
{
    HANDLE_ERROR(SDL_SetSurfaceBlendMode(Get_SDL_Surface(self), NUM2INT(mode)));
    return mode;
}

/*
 * Return true if the surface need to lock when you get access to the
 * pixel data of the surface.
 *
 * @see #lock
 */
static VALUE Surface_must_lock_p(VALUE self)
{
    return INT2BOOL(SDL_MUSTLOCK(Get_SDL_Surface(self)));
}

/*
 * Lock the surface.
 *
 * @return [nil]
 * 
 * @see #unlock
 * @see #must_lock?
 */
static VALUE Surface_lock(VALUE self)
{
    HANDLE_ERROR(SDL_LockSurface(Get_SDL_Surface(self)));
    return Qnil;
}

/*
 * Unlock the surface.
 *
 * @return [nil]
 * 
 * @see #lock
 */
static VALUE Surface_unlock(VALUE self)
{
    SDL_UnlockSurface(Get_SDL_Surface(self));
    return Qnil;
}

/*
 * @overload pixel(x, y) 
 *   Get a pixel data at (**x**, **y**)
 *
 *   @param x [Integer] the x coordinate
 *   @param y [Integer] the y coordinate
 *   @return [Integer] pixel data
 *
 *   @see #pixel_color
 *   
 */
static VALUE Surface_pixel(VALUE self, VALUE x_coord, VALUE y_coord)
{
    int x = NUM2INT(x_coord);
    int y = NUM2INT(y_coord);
    SDL_Surface* surface = Get_SDL_Surface(self);
    int offset;
    Uint32 pixel = 0;
    int i;
    
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h)
        rb_raise(rb_eArgError, "(%d, %d) out of range for %dx%d",
                 x, y, surface->w, surface->h);
    offset = surface->pitch * y + surface->format->BytesPerPixel * x;
    for (i=0; i<surface->format->BytesPerPixel; ++i) {
        pixel += *((Uint8*)surface->pixels + offset + i) << (8*i);
    }

    return UINT2NUM(SDL_SwapLE32(pixel));
}

/*
 * Get all pixel data of the surface as a string.
 *
 * @return [String]
 *
 */
static VALUE Surface_pixels(VALUE self)
{
    SDL_Surface* surface = Get_SDL_Surface(self);
    int size = surface->h * surface->pitch;
    return rb_str_new(surface->pixels, size);
}

/*
 * Get the pitch (bytes per horizontal line) of the surface.
 *
 * @return [Integer]
 */
static VALUE Surface_pitch(VALUE self)
{
    return UINT2NUM(Get_SDL_Surface(self)->pitch);
}

/*
 * Get bits per pixel of the surface.
 *
 * @return [Integer]
 */
static VALUE Surface_bits_per_pixel(VALUE self)
{
    return UCHAR2NUM(Get_SDL_Surface(self)->format->BitsPerPixel);
}

/*
 * Get bytes per pixel of the surface.
 *
 * @return [Integer]
 */
static VALUE Surface_bytes_per_pixel(VALUE self)
{
    return UCHAR2NUM(Get_SDL_Surface(self)->format->BytesPerPixel);
}

/*
 * @overload pixel_color(x, y)
 *   Get the pixel color (r,g,b and a) at (**x**, **y**) of the surface.
 *
 *   @param x [Integer] the x coordinate
 *   @param y [Integer] the y coordinate
 *   @return [[Integer, Integer, Integer, Integer]]
 *     the red, green, blue, and alpha component of the specified pixel.
 *
 */
static VALUE Surface_pixel_color(VALUE self, VALUE x, VALUE y)
{
    Uint32 pixel = NUM2UINT(Surface_pixel(self, x, y));
    SDL_Surface* surface = Get_SDL_Surface(self);
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

    return rb_ary_new3(4, UCHAR2NUM(r), UCHAR2NUM(g), UCHAR2NUM(b), UCHAR2NUM(a));
}

static Uint32 pixel_value(VALUE val, SDL_PixelFormat* format)
{
    if (RTEST(rb_funcall(val, rb_intern("integer?"), 0, 0))) {
        return NUM2UINT(val);
    } else {
        long len;
        Uint8 r, g, b, a;
        Check_Type(val, T_ARRAY);
        len = RARRAY_LEN(val);
        if (len == 3 || len == 4) {
            r = NUM2UCHAR(rb_ary_entry(val, 0));
            g = NUM2UCHAR(rb_ary_entry(val, 1));
            b = NUM2UCHAR(rb_ary_entry(val, 2));
            if (len == 3)
                a = 255;
            else
                a = NUM2UCHAR(rb_ary_entry(val, 3));
            return SDL_MapRGBA(format, r, g, b, a);
        } else {
            rb_raise(rb_eArgError, "Wrong length of array (%ld for 3..4)", len);
        }
    }
    return 0;
}

/*
 * Unset the color key of the surface.
 *
 * @return [nil]
 *
 * @see #color_key=
 */
static VALUE Surface_unset_color_key(VALUE self)
{
    HANDLE_ERROR(SDL_SetColorKey(Get_SDL_Surface(self), SDL_FALSE, 0));
    return Qnil;
}

/*
 * @overload color_key=(key) 
 *   Set the color key of the surface
 *
 *   @param key [Integer, Array<Integer>]
 *     the color key, pixel value (see {#pixel}) or pixel color (array of
 *     three or four integer elements).
 *
 *   @return [key]
 *
 *   @see #color_key
 *   @see #unset_color_key
 */
static VALUE Surface_set_color_key(VALUE self, VALUE key)
{
    SDL_Surface* surface = Get_SDL_Surface(self);
    if (key == Qnil)
        return Surface_unset_color_key(self);
    
    HANDLE_ERROR(SDL_SetColorKey(surface, SDL_TRUE, pixel_value(key, surface->format)));
    
    return key;
}

/*
 * Get the color key of the surface
 *
 * @return [Integer] the color key, as pixel value (see {#pixel})
 *
 * @see #color_key=
 * @see #unset_color_key
 */
static VALUE Surface_color_key(VALUE self)
{
    Uint32 key;
    if (SDL_GetColorKey(Get_SDL_Surface(self), &key) < 0)
        return Qnil;
    else
        return UINT2NUM(key);
}

/*
 * Get the width of the surface.
 *
 * @return [Integer]
 */
static VALUE Surface_w(VALUE self)
{
    return INT2NUM(Get_SDL_Surface(self)->w);
}

/*
 * Get the height of the surface.
 *
 * @return [Integer]
 */
static VALUE Surface_h(VALUE self)
{
    return INT2NUM(Get_SDL_Surface(self)->h);
}


/*
 * @overload blit(src, srcrect, dst, dstrect)
 *   Perform a fast blit from **src** surface to **dst** surface.
 *
 *   @param src [SDL2::Surface] the source surface
 *   @param srcrect [SDL2::Rect,nil] the region in the source surface,
 *     if nil is given, the whole source is used
 *   @param dst [SDL2::Surface] the destination surface
 *   @param dstrect [SDL2::Rect,nil] the region in the destination surface
 *     if nil is given, the source image is copied to (0, 0) on
 *     the destination surface.
 *     **dstrect** is changed by this method to store the
 *     actually copied region (since the surface has clipping functionality,
 *     actually copied region may be different from given **dstrect**).
 *   @return [nil]
 */
static VALUE Surface_s_blit(VALUE self, VALUE src, VALUE srcrect, VALUE dst, VALUE dstrect)
{
    HANDLE_ERROR(SDL_BlitSurface(Get_SDL_Surface(src),
                                 Get_SDL_Rect_or_NULL(srcrect),
                                 Get_SDL_Surface(dst),
                                 Get_SDL_Rect_or_NULL(dstrect)));
    return Qnil;
}

/*
 * Create an empty RGB surface.
 *
 * If rmask, gmask, bmask are omitted, the default masks are used.
 * If amask is omitted, alpha mask is considered to be zero.
 *
 * @overload new(width, height, depth)
 * @overload new(width, height, depth, amask)
 * @overload new(width, heigth, depth, rmask, gmask, bmask, amask)
 *
 * @param width [Integer] the width of the new surface
 * @param height [Integer] the height of the new surface
 * @param depth [Integer] the bit depth of the new surface
 * @param rmask [Integer] the red mask of a pixel
 * @param gmask [Integer] the green mask of a pixel
 * @param bmask [Integer] the blue mask of a pixel
 * @param amask [Integer] the alpha mask of a pixel
 * 
 * @return [SDL2::Surface]
 */
static VALUE Surface_s_new(int argc, VALUE* argv, VALUE self)
{
    VALUE width, height, depth;
    Uint32 Rmask, Gmask, Bmask, Amask;
    SDL_Surface * surface;
    
    if  (argc == 3) {
        rb_scan_args(argc, argv, "30", &width, &height, &depth);
        Rmask = Gmask = Bmask = Amask = 0;
    } else if (argc == 7) {
        VALUE rm, gm, bm, am;
        rb_scan_args(argc, argv, "70", &width, &height, &depth, &rm, &gm, &bm, &am);
        Rmask = NUM2UINT(rm); Gmask = NUM2UINT(gm); 
        Bmask = NUM2UINT(bm); Amask = NUM2UINT(am);
    } else {
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 4 or 7)", argc);
    }

    surface = SDL_CreateRGBSurface(0, NUM2INT(width), NUM2INT(height), NUM2INT(depth),
                                   Rmask, Gmask, Bmask, Amask);
    if (!surface)
        SDL_ERROR();

    return Surface_new(surface);
}

#define FIELD_ACCESSOR(classname, typename, field)              \
    static VALUE classname##_##field(VALUE self)                \
    {                                                           \
        typename* r; Data_Get_Struct(self, typename, r);        \
        return INT2NUM(r->field);                               \
    }                                                           \
    static VALUE classname##_set_##field(VALUE self, VALUE val) \
    {                                                           \
        typename* r; Data_Get_Struct(self, typename, r);        \
        r->field = NUM2INT(val); return val;                    \
    }


/* Document-class: SDL2::Rect
 *
 * This class represents a rectangle in SDL2.
 *
 * Any rectanle is represented by four attributes x, y, w, and h,
 * and these four attributes must be integer.
 *
 * @!attribute [rw] x
 *   X coordiante of the left-top point of the rectangle
 *   @return [Integer]
 *
 * @!attribute [rw] y
 *   Y coordiante of the left-top point of the rectangle
 *   @return [Integer]
 *
 * @!attribute [rw] w
 *   Width of the rectangle
 *   @return [Integer]
 *
 * @!attribute [rw] h
 *   Height of the rectangle
 *   @return [Integer]
 *
 * @!method self.[](*args)
 *   Alias of new. See {#initialize}.
 *   @return [SDL2::Rect]
 */
static VALUE Rect_s_allocate(VALUE klass)
{
    SDL_Rect* rect = ALLOC(SDL_Rect);
    rect->x = rect->y = rect->w = rect->h = 0;
  
    return Data_Wrap_Struct(cRect, 0, free, rect);
}

/* 
 * Create a new SDL2::Rect object
 *
 * @return [SDL2::Rect]
 * 
 * @overload initialze(x, y, w, h)
 *   Create a new SDL2::Rect object
 *   
 *   @param x [Integer] X coordiante of the left-top point of the rectangle
 *   @param y [Integer] Y coordiante of the left-top point of the rectangle
 *   @param w [Integer] Width of the rectangle
 *   @param h [Integer] Height of the rectangle
 *   
 * @overload initialize
 *   Create a new SDL2::Rect object whose x, w, w, and h are all
 *   zero.
 */
static VALUE Rect_initialize(int argc, VALUE* argv, VALUE self)
{
    VALUE x, y, w, h;
    rb_scan_args(argc, argv, "04", &x, &y, &w, &h);
    if (argc == 0) {
        /* do nothing*/
    } else if (argc == 4) {
        SDL_Rect* rect;
        Data_Get_Struct(self, SDL_Rect, rect);
        rect->x = NUM2INT(x); rect->y = NUM2INT(y);
        rect->w = NUM2INT(w); rect->h = NUM2INT(h);
    } else {
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 0 or 4)", argc);
    }
    return Qnil;
}

/*
 * Inspection string for debug
 * @return [String]
 */
static VALUE Rect_inspect(VALUE self)
{
    SDL_Rect* rect = Get_SDL_Rect(self);
    return rb_sprintf("<SDL2::Rect: x=%d y=%d w=%d h=%d>",
                      rect->x, rect->y, rect->w, rect->h);
}

FIELD_ACCESSOR(Rect, SDL_Rect, x);
FIELD_ACCESSOR(Rect, SDL_Rect, y);
FIELD_ACCESSOR(Rect, SDL_Rect, w);
FIELD_ACCESSOR(Rect, SDL_Rect, h);

/*
 * @overload intersection(other)
 *   Returns the intersection rect of self and other.
 *
 *   If there is no intersection, returns nil.
 *   
 *   @return [SDL2::Rect, nil]
 *   @param [SDL2::Rect] other rectangle
 */
static VALUE Rect_intersection(VALUE self, VALUE other)
{
    VALUE result = Rect_s_allocate(cRect);
    if (SDL_IntersectRect(Get_SDL_Rect(self), Get_SDL_Rect(other), Get_SDL_Rect(result))) {
        return result;
    } else {
        return Qnil;
    }
}

/*
 * @overload union(other)
 *   Returns the minimal rect containing self and other
 *
 *   @return [SDL2::Rect]
 *   @param [SDL2::Rect] other rectangle
 */
static VALUE Rect_union(VALUE self, VALUE other)
{
    VALUE result = Rect_s_allocate(cRect);
    SDL_UnionRect(Get_SDL_Rect(self), Get_SDL_Rect(other), Get_SDL_Rect(result));
    return result;
}

/* 
 * Document-class: SDL2::Point
 *
 * This class represents a point in SDL library. 
 * Some method requires this method.
 *
 * @!attribute [rw] x
 *   X coordiante of the point.
 *   @return [Integer]
 *
 * @!attribute [rw] y
 *   Y coordiante of the point.
 *   @return [Integer]
 */
static VALUE Point_s_allocate(VALUE klass)
{
    SDL_Point* point;
    return Data_Make_Struct(klass, SDL_Point, 0, free, point);
}

/*
 * Create a new point object.
 * 
 * @overload initialize(x, y)
 *   @param x the x coordinate of the point
 *   @param y the y coordinate of the point
 *   
 * @overload initialize
 *   x and y of the created point object are initialized by 0
 *
 * @return [SDL2::Point]
 * 
 */
static VALUE Point_initialize(int argc, VALUE* argv, VALUE self)
{
    VALUE x, y;
    SDL_Point* point = Get_SDL_Point(self);
    rb_scan_args(argc, argv, "02", &x, &y);
    point->x = (x == Qnil) ? 0 : NUM2INT(x);
    point->y = (y == Qnil) ? 0 : NUM2INT(y);
    return Qnil;
}

/*
 * Return inspection string.
 * @return [String]
 */
static VALUE Point_inspect(VALUE self)
{
    SDL_Point* point = Get_SDL_Point(self);
    return rb_sprintf("<SDL2::Point x=%d y=%d>", point->x, point->y);
}

FIELD_ACCESSOR(Point, SDL_Point, x);
FIELD_ACCESSOR(Point, SDL_Point, y);


/*
 * Document-class: SDL2::PixelFormat
 *
 * This class represents pixel format of textures, windows, and displays.
 *
 * In C level, SDL use unsigned integers as pixel formats. This class
 * wraps these integers. You can get the integers from {#format}.
 *
 * @!attribute [r] format
 *    An integer representing the pixel format.
 *
 *    @return [Integer]
 */

/*
 * @overload initialze(format)
 *
 *   Initialize pixel format from the given integer representing a fomrmat.
 *   
 *   @param format [Integer] an unsigned integer as a pixel formats
 */
static VALUE PixelForamt_initialize(VALUE self, VALUE format)
{
    rb_iv_set(self, "@format", format);
    return Qnil;
}

/*
 * Get the type of the format.
 *
 * @return [Integer] One of the constants of {Type} module.
 */
static VALUE PixelFormat_type(VALUE self)
{
    return UINT2NUM(SDL_PIXELTYPE(NUM2UINT(rb_iv_get(self, "@format"))));
}

/*
define(`PIXELFORMAT_ATTR_READER',
`static VALUE PixelFormat_$1(VALUE self)
{
    return $3($2(NUM2UINT(rb_iv_get(self, "@format"))));
}')
 */

/*
 * Get the human readable name of the pixel format
 * 
 * @return [String]
 */
PIXELFORMAT_ATTR_READER(name, SDL_GetPixelFormatName, utf8str_new_cstr);

/*
 * Get the ordering of channels or bits in the pixel format.
 *
 * @return [Integer] One of the constants of {BitmapOrder} module or {PackedOrder} module.
 */
PIXELFORMAT_ATTR_READER(order,  SDL_PIXELORDER, UINT2NUM);

/*
 * Get the channel bit pattern of the pixel format.
 *
 * @return [Integer] One of the constants of {PackedLayout} module.
 */
PIXELFORMAT_ATTR_READER(layout,  SDL_PIXELLAYOUT, UINT2NUM);

/*
 * Get the number of bits per pixel.
 *
 * @return [Integer]
 */
PIXELFORMAT_ATTR_READER(bits_per_pixel,  SDL_BITSPERPIXEL, INT2NUM);

/*
 * Get the number of bytes per pixel.
 *
 * @return [Integer]
 */
PIXELFORMAT_ATTR_READER(bytes_per_pixel,  SDL_BYTESPERPIXEL, INT2NUM);

/*
 * Return true if the pixel format have a palette.
 */
PIXELFORMAT_ATTR_READER(indexed_p,  SDL_ISPIXELFORMAT_INDEXED, INT2BOOL);

/*
 * Return true if the pixel format have an alpha channel.
 */
PIXELFORMAT_ATTR_READER(alpha_p,  SDL_ISPIXELFORMAT_ALPHA, INT2BOOL);

/*
 * Return true if the pixel format is not indexed, and not RGB(A),
 * for example, the pixel format is YUV.
 */
PIXELFORMAT_ATTR_READER(fourcc_p,  SDL_ISPIXELFORMAT_FOURCC, INT2BOOL);

/* 
 * @overload ==(other)
 *   Return true if two pixel format is the same format.
 *
 * @return [Boolean]
 */
static VALUE PixelFormat_eq(VALUE self, VALUE other)
{
    if (!rb_obj_is_kind_of(other, cPixelFormat))
        return Qfalse;

    return INT2BOOL(rb_iv_get(self, "@format") == rb_iv_get(other, "@format"));
}

/* @return [String] inspection string */
static VALUE PixelFormat_inspect(VALUE self)
{
    Uint32 format = NUM2UINT(rb_iv_get(self, "@format"));
    return rb_sprintf("<%s: %s type=%d order=%d layout=%d"
                      " bits=%d bytes=%d indexed=%s alpha=%s fourcc=%s>",
                      rb_obj_classname(self),
                      SDL_GetPixelFormatName(format),
                      SDL_PIXELTYPE(format), SDL_PIXELORDER(format), SDL_PIXELLAYOUT(format),
                      SDL_BITSPERPIXEL(format), SDL_BYTESPERPIXEL(format),
                      INT2BOOLCSTR(SDL_ISPIXELFORMAT_INDEXED(format)),
                      INT2BOOLCSTR(SDL_ISPIXELFORMAT_ALPHA(format)),
                      INT2BOOLCSTR(SDL_ISPIXELFORMAT_FOURCC(format)));
}

/*
 * Document-module: SDL2::ScreenSaver
 *
 * This module provides functions to disable and enable a screensaver.
 */

/*
 * Enable screensaver.
 *
 * @return [nil]
 * @see .disable
 * @see .enabled?
 */
static VALUE ScreenSaver_enable(VALUE self)
{
    SDL_EnableScreenSaver();
    return Qnil;
}

/*
 * Disable screensaver.
 *
 * @return [nil]
 * @see .enable
 * @see .enabled?
 */
static VALUE ScreenSaver_disable(VALUE self)
{
    SDL_DisableScreenSaver();
    return Qnil;
}

/*
 * Return true if the screensaver is enabled.
 *
 * @see .enable
 * @see .disable
 */
static VALUE ScreenSaver_enabled_p(VALUE self)
{
    return INT2BOOL(SDL_IsScreenSaverEnabled());
}

/*
define(`DEFINE_C_ACCESSOR',`rb_define_method($2, "$3", $1_$3, 0);
    rb_define_method($2, "$3=", $1_set_$3, 1)')
 */
    
void rubysdl2_init_video(void)
{
    rb_define_module_function(mSDL2, "video_drivers", SDL2_s_video_drivers, 0);
    rb_define_module_function(mSDL2, "current_video_driver", SDL2_s_current_video_driver, 0);
    rb_define_module_function(mSDL2, "video_init", SDL2_s_video_init, 1);
    
    cWindow = rb_define_class_under(mSDL2, "Window", rb_cObject);
    
    rb_undef_alloc_func(cWindow);
    rb_define_singleton_method(cWindow, "create", Window_s_create, 6);
    rb_define_singleton_method(cWindow, "all_windows", Window_s_all_windows, 0);
    rb_define_singleton_method(cWindow, "find_by_id", Window_s_find_by_id, 1);
    rb_define_method(cWindow, "destroy?", Window_destroy_p, 0);
    rb_define_method(cWindow, "destroy", Window_destroy, 0);
    rb_define_method(cWindow, "create_renderer", Window_create_renderer, 2);
    rb_define_method(cWindow, "renderer", Window_renderer, 0);
    rb_define_method(cWindow, "window_id", Window_window_id, 0);
    rb_define_method(cWindow, "inspect", Window_inspect, 0);
    rb_define_method(cWindow, "display_mode", Window_display_mode, 0);
    rb_define_method(cWindow, "display", Window_display, 0);
    rb_define_method(cWindow, "debug_info", Window_debug_info, 0);
    DEFINE_C_ACCESSOR(Window, cWindow, brightness);
    rb_define_method(cWindow, "flags", Window_flags, 0);
    rb_define_method(cWindow, "gamma_ramp", Window_gamma_ramp, 0);
    rb_define_method(cWindow, "input_is_grabbed?", Window_input_is_grabbed_p, 0);
    rb_define_method(cWindow, "input_is_grabbed=", Window_set_input_is_grabbed, 1);
    DEFINE_C_ACCESSOR(Window, cWindow, maximum_size);
    DEFINE_C_ACCESSOR(Window, cWindow, minimum_size);
    DEFINE_C_ACCESSOR(Window, cWindow, position);
    DEFINE_C_ACCESSOR(Window, cWindow, size);
    DEFINE_C_ACCESSOR(Window, cWindow, title);
    DEFINE_C_ACCESSOR(Window, cWindow, bordered);
    rb_define_method(cWindow, "icon=", Window_set_icon, 1);
    rb_define_method(cWindow, "show", Window_show, 0);
    rb_define_method(cWindow, "hide", Window_hide, 0);
    rb_define_method(cWindow, "maximize", Window_maximize, 0);
    rb_define_method(cWindow, "minimize", Window_minimize, 0);
    rb_define_method(cWindow, "raise", Window_raise, 0);
    rb_define_method(cWindow, "restore", Window_restore, 0);
    rb_define_method(cWindow, "fullscreen_mode", Window_fullscreen_mode, 0);
    rb_define_method(cWindow, "fullscreen_mode=", Window_set_fullscreen_mode, 1);
#if SDL_VERSION_ATLEAST(2,0,1)
    rb_define_method(cWindow, "gl_drawable_size", Window_gl_drawable_size, 0);
#endif
    rb_define_method(cWindow, "gl_swap", Window_gl_swap, 0);

    /* Indicate that you don't care what the window position is */
    rb_define_const(cWindow, "POS_CENTERED", INT2NUM(SDL_WINDOWPOS_CENTERED));
    /* Indicate that the window position should be centered */
    rb_define_const(cWindow, "POS_UNDEFINED", INT2NUM(SDL_WINDOWPOS_UNDEFINED));

    mWindowFlags = rb_define_module_under(cWindow, "Flags");
    /* define(`DEFINE_WINDOW_FLAGS_CONST',`rb_define_const(mWindowFlags, "$1", UINT2NUM(SDL_WINDOW_$1))') */
    /* fullscreen window */ 
    DEFINE_WINDOW_FLAGS_CONST(FULLSCREEN);
    /* fullscreen window at the current desktop resolution */
    DEFINE_WINDOW_FLAGS_CONST(FULLSCREEN_DESKTOP);
    /* window usable with OpenGL context */ 
    DEFINE_WINDOW_FLAGS_CONST(OPENGL);
    /* window is visible */
    DEFINE_WINDOW_FLAGS_CONST(SHOWN);
    /* window is not visible */
    DEFINE_WINDOW_FLAGS_CONST(HIDDEN);
    /* no window decoration */
    DEFINE_WINDOW_FLAGS_CONST(BORDERLESS);
    /* window is resizable */ 
    DEFINE_WINDOW_FLAGS_CONST(RESIZABLE);
    /* window is minimized */
    DEFINE_WINDOW_FLAGS_CONST(MINIMIZED);
    /* window is maximized */
    DEFINE_WINDOW_FLAGS_CONST(MAXIMIZED);
    /* window has grabbed input focus */
    DEFINE_WINDOW_FLAGS_CONST(INPUT_GRABBED);
    /* window has input focus */
    DEFINE_WINDOW_FLAGS_CONST(INPUT_FOCUS);
    /* window has mouse focus */
    DEFINE_WINDOW_FLAGS_CONST(MOUSE_FOCUS);
    /* window is not created by SDL */
    DEFINE_WINDOW_FLAGS_CONST(FOREIGN);
#ifdef SDL_WINDOW_ALLOW_HIGHDPI
    /* window should be created in high-DPI mode if supported (>= SDL 2.0.1)*/
    DEFINE_WINDOW_FLAGS_CONST(ALLOW_HIGHDPI);
#endif
#ifdef SDL_WINDOW_MOSUE_CAPTURE
    /* window has mouse caputred (>= SDL 2.0.4) */
    DEFINE_WINDOW_FLAGS_CONST(MOUSE_CAPTURE);
#endif

    cDisplay = rb_define_class_under(mSDL2, "Display", rb_cObject);
    
    rb_define_module_function(cDisplay, "displays", Display_s_displays, 0);
    rb_define_attr(cDisplay, "index", 1, 0);
    rb_define_attr(cDisplay, "name", 1, 0);
    rb_define_method(cDisplay, "modes", Display_modes, 0); 
    rb_define_method(cDisplay, "current_mode", Display_current_mode, 0);
    rb_define_method(cDisplay, "desktop_mode", Display_desktop_mode, 0);
    rb_define_method(cDisplay, "closest_mode", Display_closest_mode, 1);
    rb_define_method(cDisplay, "bounds", Display_bounds, 0);

    
    cDisplayMode = rb_define_class_under(cDisplay, "Mode", rb_cObject);

    rb_define_alloc_func(cDisplayMode, DisplayMode_s_allocate);
    rb_define_method(cDisplayMode, "initialize", DisplayMode_initialize, 4);
    rb_define_method(cDisplayMode, "inspect", DisplayMode_inspect, 0);
    rb_define_method(cDisplayMode, "format", DisplayMode_format, 0);
    rb_define_method(cDisplayMode, "w", DisplayMode_w, 0);
    rb_define_method(cDisplayMode, "h", DisplayMode_h, 0);
    rb_define_method(cDisplayMode, "refresh_rate", DisplayMode_refresh_rate, 0);
    
    
    cRenderer = rb_define_class_under(mSDL2, "Renderer", rb_cObject);
    
    rb_undef_alloc_func(cRenderer);
    rb_define_singleton_method(cRenderer, "drivers_info", Renderer_s_drivers_info, 0);
    rb_define_method(cRenderer, "destroy?", Renderer_destroy_p, 0);
    rb_define_method(cRenderer, "destroy", Renderer_destroy, 0);
    rb_define_method(cRenderer, "debug_info", Renderer_debug_info, 0);
    rb_define_method(cRenderer, "create_texture", Renderer_create_texture, 4);
    rb_define_method(cRenderer, "create_texture_from", Renderer_create_texture_from, 1);
    rb_define_method(cRenderer, "copy", Renderer_copy, 3);
    rb_define_method(cRenderer, "copy_ex", Renderer_copy_ex, 6);
    rb_define_method(cRenderer, "present", Renderer_present, 0);
    rb_define_method(cRenderer, "draw_color",Renderer_draw_color, 0);
    rb_define_method(cRenderer, "draw_color=",Renderer_set_draw_color, 1);
    rb_define_method(cRenderer, "clear", Renderer_clear, 0);
    rb_define_method(cRenderer, "draw_line",Renderer_draw_line, 4);
    rb_define_method(cRenderer, "draw_point",Renderer_draw_point, 2);
    rb_define_method(cRenderer, "draw_rect", Renderer_draw_rect, 1);
    rb_define_method(cRenderer, "fill_rect", Renderer_fill_rect, 1);
    rb_define_method(cRenderer, "draw_blend_mode", Renderer_draw_blend_mode, 0);
    rb_define_method(cRenderer, "draw_blend_mode=", Renderer_set_draw_blend_mode, 1);
    rb_define_method(cRenderer, "clip_rect", Renderer_clip_rect, 0);
    rb_define_method(cRenderer, "clip_rect=", Renderer_set_clip_rect, 1);
#if SDL_VERSION_ATLEAST(2,0,4)
    rb_define_method(cRenderer, "clip_enabled?", Render_clip_enabled_p, 0);
#endif
    rb_define_method(cRenderer, "logical_size", Renderer_logical_size, 0);
    rb_define_method(cRenderer, "logical_size=", Renderer_set_logical_size, 1);
    rb_define_method(cRenderer, "scale", Renderer_scale, 0);
    rb_define_method(cRenderer, "scale=", Renderer_set_scale, 1);
    rb_define_method(cRenderer, "viewport", Renderer_viewport, 0);
    rb_define_method(cRenderer, "viewport=", Renderer_set_viewport, 1);
    rb_define_method(cRenderer, "support_render_target?", Renderer_support_render_target_p, 0);
    rb_define_method(cRenderer, "output_size", Renderer_output_size, 0);
    rb_define_method(cRenderer, "render_target", Renderer_render_target, 0);
    rb_define_method(cRenderer, "render_target=", Renderer_set_render_target, 1);
    rb_define_method(cRenderer, "reset_render_target", Renderer_reset_render_target, 0);
    
    rb_define_method(cRenderer, "info", Renderer_info, 0);

    mRendererFlags = rb_define_module_under(cRenderer, "Flags");
    
    /* define(`DEFINE_RENDERER_FLAGS_CONST',`rb_define_const(mRendererFlags, "$1", UINT2NUM(SDL_RENDERER_$1))') */
    /* the renderer is a software fallback */
    DEFINE_RENDERER_FLAGS_CONST(SOFTWARE);
    /* the renderer uses hardware acceleration */
    DEFINE_RENDERER_FLAGS_CONST(ACCELERATED);
#ifdef SDL_RENDERER_PRESENTVSYNC
    /* present is synchronized with the refresh rate */
    DEFINE_RENDERER_FLAGS_CONST(PRESENTVSYNC);
#endif
    /* the renderer supports rendering to texture */ 
    DEFINE_RENDERER_FLAGS_CONST(TARGETTEXTURE);
    /* define(`DEFINE_SDL_FLIP_CONST',`rb_define_const(cRenderer, "FLIP_$1", INT2FIX(SDL_FLIP_$1))') */
    /* Do not flip, used in {Renderer#copy_ex} */
    DEFINE_SDL_FLIP_CONST(NONE);
    /* Flip horizontally, used in {Renderer#copy_ex} */
    DEFINE_SDL_FLIP_CONST(HORIZONTAL);
    /* Flip vertically, used in {Renderer#copy_ex} */
    DEFINE_SDL_FLIP_CONST(VERTICAL);
    
    mBlendMode = rb_define_module_under(mSDL2, "BlendMode");
    /* define(`DEFINE_BLENDMODE_CONST',`rb_define_const(mBlendMode, "$1", INT2FIX(SDL_BLENDMODE_$1))') */
    /* no blending (dstRGBA = srcRGBA) */
    DEFINE_BLENDMODE_CONST(NONE);
    /* alpha blending (dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA), dstA = srcA + (dstA * (1-srcA)))*/
    DEFINE_BLENDMODE_CONST(BLEND);
    /* additive blending (dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA) */
    DEFINE_BLENDMODE_CONST(ADD);
    /* color modulate (multiplicative) (dstRGB = srcRGB * dstRGB, dstA = dstA) */
    DEFINE_BLENDMODE_CONST(MOD);
    
    cTexture = rb_define_class_under(mSDL2, "Texture", rb_cObject);
    
    rb_undef_alloc_func(cTexture);
    rb_define_method(cTexture, "destroy?", Texture_destroy_p, 0);
    rb_define_method(cTexture, "destroy", Texture_destroy, 0);
    DEFINE_C_ACCESSOR(Texture, cTexture, blend_mode);
    DEFINE_C_ACCESSOR(Texture, cTexture, color_mod);
    DEFINE_C_ACCESSOR(Texture, cTexture, alpha_mod);
    rb_define_method(cTexture, "format", Texture_format, 0);
    rb_define_method(cTexture, "access_pattern", Texture_access_pattern, 0);
    rb_define_method(cTexture, "w", Texture_w, 0);
    rb_define_method(cTexture, "h", Texture_h, 0);
    rb_define_method(cTexture, "inspect", Texture_inspect, 0);
    rb_define_method(cTexture, "debug_info", Texture_debug_info, 0);
    /* define(`DEFINE_TEXTUREAH_ACCESS_CONST', `rb_define_const(cTexture, "ACCESS_$1", INT2NUM(SDL_TEXTUREACCESS_$1))') */
    /* texture access pattern - changes rarely, not lockable */
    DEFINE_TEXTUREAH_ACCESS_CONST(STATIC);
    /* texture access pattern - changes frequently, lockable */
    DEFINE_TEXTUREAH_ACCESS_CONST(STREAMING);
    /* texture access pattern - can be used as a render target */
    DEFINE_TEXTUREAH_ACCESS_CONST(TARGET);

    
    cSurface = rb_define_class_under(mSDL2, "Surface", rb_cObject);
    
    rb_undef_alloc_func(cSurface);
    rb_define_singleton_method(cSurface, "load_bmp", Surface_s_load_bmp, 1);
    rb_define_singleton_method(cSurface, "blit", Surface_s_blit, 4);
    rb_define_singleton_method(cSurface, "new", Surface_s_new, -1);
    rb_define_singleton_method(cSurface, "from_string", Surface_s_from_string, -1);
    rb_define_method(cSurface, "destroy?", Surface_destroy_p, 0);
    rb_define_method(cSurface, "destroy", Surface_destroy, 0);
    DEFINE_C_ACCESSOR(Surface, cSurface, blend_mode);
    rb_define_method(cSurface, "must_lock?", Surface_must_lock_p, 0);
    rb_define_method(cSurface, "lock", Surface_lock, 0);
    rb_define_method(cSurface, "unlock", Surface_unlock, 0);
    rb_define_method(cSurface, "w", Surface_w, 0);
    rb_define_method(cSurface, "h", Surface_h, 0);
    rb_define_method(cSurface, "pixel", Surface_pixel, 2);
    rb_define_method(cSurface, "pixel_color", Surface_pixel_color, 2);
    rb_define_method(cSurface, "color_key", Surface_color_key, 0);
    rb_define_method(cSurface, "color_key=", Surface_set_color_key, 1);
    rb_define_method(cSurface, "unset_color_key", Surface_set_color_key, 0);
    rb_define_method(cSurface, "pixels", Surface_pixels, 0);
    rb_define_method(cSurface, "pitch", Surface_pitch, 0);
    rb_define_method(cSurface, "bits_per_pixel", Surface_bits_per_pixel, 0);
    rb_define_method(cSurface, "bytes_per_pixel", Surface_bytes_per_pixel, 0);
    
    cRect = rb_define_class_under(mSDL2, "Rect", rb_cObject);

    rb_define_alloc_func(cRect, Rect_s_allocate);
    rb_define_method(cRect, "initialize", Rect_initialize, -1);
    rb_define_alias(rb_singleton_class(cRect), "[]", "new");
    rb_define_method(cRect, "inspect", Rect_inspect, 0);
    DEFINE_C_ACCESSOR(Rect, cRect, x);
    DEFINE_C_ACCESSOR(Rect, cRect, y);
    DEFINE_C_ACCESSOR(Rect, cRect, w);
    DEFINE_C_ACCESSOR(Rect, cRect, h);
    rb_define_method(cRect, "union", Rect_union, 1);
    rb_define_method(cRect, "intersection", Rect_intersection, 1);
    
    cPoint = rb_define_class_under(mSDL2, "Point", rb_cObject);

    rb_define_alloc_func(cPoint, Point_s_allocate);
    rb_define_method(cPoint, "initialize", Point_initialize, -1);
    rb_define_alias(rb_singleton_class(cPoint), "[]", "new");
    rb_define_method(cPoint, "inspect", Point_inspect, 0);
    DEFINE_C_ACCESSOR(Point, cPoint, x);
    DEFINE_C_ACCESSOR(Point, cPoint, y);

    
    cRendererInfo = rb_define_class_under(cRenderer, "Info", rb_cObject);
    define_attr_readers(cRendererInfo, "name", "flags", "texture_formats",
                        "max_texture_width", "max_texture_height", NULL);
    
    
    cPixelFormat = rb_define_class_under(mSDL2, "PixelFormat", rb_cObject);
    
    rb_define_method(cPixelFormat, "initialize", PixelForamt_initialize, 1);
    rb_define_attr(cPixelFormat, "format", 1, 0);
    rb_define_method(cPixelFormat, "name", PixelFormat_name, 0);
    rb_define_method(cPixelFormat, "inspect", PixelFormat_inspect, 0);
    rb_define_method(cPixelFormat, "type", PixelFormat_type, 0);
    rb_define_method(cPixelFormat, "order", PixelFormat_order, 0);
    rb_define_method(cPixelFormat, "layout", PixelFormat_layout, 0);
    rb_define_method(cPixelFormat, "bits_per_pixel", PixelFormat_bits_per_pixel, 0);
    rb_define_alias(cPixelFormat, "bpp", "bits_per_pixel");
    rb_define_method(cPixelFormat, "bytes_per_pixel", PixelFormat_bytes_per_pixel, 0);
    rb_define_method(cPixelFormat, "indexed?", PixelFormat_indexed_p, 0);
    rb_define_method(cPixelFormat, "alpha?", PixelFormat_alpha_p, 0);
    rb_define_method(cPixelFormat, "fourcc?", PixelFormat_fourcc_p, 0);
    rb_define_method(cPixelFormat, "==", PixelFormat_eq, 1);

    mPixelType = rb_define_module_under(cPixelFormat, "Type");
    /* define(`DEFINE_PIXELTYPE_CONST',`rb_define_const(mPixelType, "$1", UINT2NUM(SDL_PIXELTYPE_$1))') */
    DEFINE_PIXELTYPE_CONST(UNKNOWN);
    DEFINE_PIXELTYPE_CONST(INDEX1);
    DEFINE_PIXELTYPE_CONST(INDEX4);
    DEFINE_PIXELTYPE_CONST(INDEX8);
    DEFINE_PIXELTYPE_CONST(PACKED8);
    DEFINE_PIXELTYPE_CONST(PACKED16);
    DEFINE_PIXELTYPE_CONST(PACKED32);
    DEFINE_PIXELTYPE_CONST(ARRAYU8);
    DEFINE_PIXELTYPE_CONST(ARRAYU16);
    DEFINE_PIXELTYPE_CONST(ARRAYU32);
    DEFINE_PIXELTYPE_CONST(ARRAYF16);
    DEFINE_PIXELTYPE_CONST(ARRAYF32);

    mBitmapOrder = rb_define_module_under(cPixelFormat, "BitmapOrder");
    rb_define_const(mBitmapOrder, "NONE", UINT2NUM(SDL_BITMAPORDER_NONE));
    rb_define_const(mBitmapOrder, "O_1234", UINT2NUM(SDL_BITMAPORDER_1234));
    rb_define_const(mBitmapOrder, "O_4321", UINT2NUM(SDL_BITMAPORDER_4321));
    
    mPackedOrder = rb_define_module_under(cPixelFormat, "PackedOrder");
    /* define(`DEFINE_PACKEDORDER_CONST',`rb_define_const(mPackedOrder, "$1", UINT2NUM(SDL_PACKEDORDER_$1))') */
    DEFINE_PACKEDORDER_CONST(NONE);
    DEFINE_PACKEDORDER_CONST(XRGB);
    DEFINE_PACKEDORDER_CONST(RGBX);
    DEFINE_PACKEDORDER_CONST(ARGB);
    DEFINE_PACKEDORDER_CONST(RGBA);
    DEFINE_PACKEDORDER_CONST(XBGR);
    DEFINE_PACKEDORDER_CONST(BGRX);
    DEFINE_PACKEDORDER_CONST(ABGR);
    DEFINE_PACKEDORDER_CONST(BGRA);

    mArrayOrder = rb_define_module_under(cPixelFormat, "ArrayOrder");
    /* define(`DEFINE_ARRAYORDER_CONST',`rb_define_const(mArrayOrder, "$1", UINT2NUM(SDL_ARRAYORDER_$1))') */
    DEFINE_ARRAYORDER_CONST(NONE);
    DEFINE_ARRAYORDER_CONST(RGB);
    DEFINE_ARRAYORDER_CONST(RGBA);
    DEFINE_ARRAYORDER_CONST(ARGB);
    DEFINE_ARRAYORDER_CONST(BGR);
    DEFINE_ARRAYORDER_CONST(BGRA);
    DEFINE_ARRAYORDER_CONST(ABGR);

    mPackedLayout = rb_define_module_under(cPixelFormat, "PackedLayout");
    /* define(`DEFINE_PACKEDLAYOUT_CONST',`rb_define_const(mPackedLayout, "L_$1", UINT2NUM(SDL_PACKEDLAYOUT_$1))') */
    rb_define_const(mPackedLayout, "NONE", UINT2NUM(SDL_PACKEDLAYOUT_NONE));
    DEFINE_PACKEDLAYOUT_CONST(332);
    DEFINE_PACKEDLAYOUT_CONST(4444);
    DEFINE_PACKEDLAYOUT_CONST(1555);
    DEFINE_PACKEDLAYOUT_CONST(5551);
    DEFINE_PACKEDLAYOUT_CONST(565);
    DEFINE_PACKEDLAYOUT_CONST(8888);
    DEFINE_PACKEDLAYOUT_CONST(2101010);
    DEFINE_PACKEDLAYOUT_CONST(1010102);
    
    {
        VALUE formats = rb_ary_new();
        /* -: Array of all available formats */
        rb_define_const(cPixelFormat, "FORMATS", formats);
        /* define(`DEFINE_PIXELFORMAT_CONST',`do {
            VALUE format = PixelFormat_new(SDL_PIXELFORMAT_$1);
            $2
            rb_define_const(cPixelFormat, "$1", format);
            rb_ary_push(formats, format);
        } while (0)')
         */
        
        DEFINE_PIXELFORMAT_CONST(UNKNOWN, /* -: PixelFormat: Unused - reserved by SDL */);
        DEFINE_PIXELFORMAT_CONST(INDEX1LSB);
        DEFINE_PIXELFORMAT_CONST(INDEX1MSB);
        DEFINE_PIXELFORMAT_CONST(INDEX4LSB);
        DEFINE_PIXELFORMAT_CONST(INDEX4MSB);
        DEFINE_PIXELFORMAT_CONST(INDEX8);
        DEFINE_PIXELFORMAT_CONST(RGB332);
        DEFINE_PIXELFORMAT_CONST(RGB444);
        DEFINE_PIXELFORMAT_CONST(RGB555);
        DEFINE_PIXELFORMAT_CONST(BGR555);
        DEFINE_PIXELFORMAT_CONST(ARGB4444);
        DEFINE_PIXELFORMAT_CONST(RGBA4444);
        DEFINE_PIXELFORMAT_CONST(ABGR4444);
        DEFINE_PIXELFORMAT_CONST(BGRA4444);
        DEFINE_PIXELFORMAT_CONST(ARGB1555);
        DEFINE_PIXELFORMAT_CONST(RGBA5551);
        DEFINE_PIXELFORMAT_CONST(ABGR1555);
        DEFINE_PIXELFORMAT_CONST(BGRA5551);
        DEFINE_PIXELFORMAT_CONST(RGB565);
        DEFINE_PIXELFORMAT_CONST(BGR565);
        DEFINE_PIXELFORMAT_CONST(RGB24);
        DEFINE_PIXELFORMAT_CONST(BGR24);
        DEFINE_PIXELFORMAT_CONST(RGB888);
        DEFINE_PIXELFORMAT_CONST(RGBX8888);
        DEFINE_PIXELFORMAT_CONST(BGR888);
        DEFINE_PIXELFORMAT_CONST(BGRX8888);
        DEFINE_PIXELFORMAT_CONST(ARGB8888);
        DEFINE_PIXELFORMAT_CONST(RGBA8888);
        DEFINE_PIXELFORMAT_CONST(ABGR8888);
        DEFINE_PIXELFORMAT_CONST(BGRA8888);
        DEFINE_PIXELFORMAT_CONST(ARGB2101010);
        DEFINE_PIXELFORMAT_CONST(YV12);
        DEFINE_PIXELFORMAT_CONST(IYUV);
        DEFINE_PIXELFORMAT_CONST(YUY2);
        DEFINE_PIXELFORMAT_CONST(UYVY);
        DEFINE_PIXELFORMAT_CONST(YVYU);
        rb_obj_freeze(formats);
    }

    mScreenSaver = rb_define_module_under(mSDL2, "ScreenSaver");
    rb_define_module_function(mScreenSaver, "enable", ScreenSaver_enable, 0);
    rb_define_module_function(mScreenSaver, "disable", ScreenSaver_disable, 0);
    rb_define_module_function(mScreenSaver, "enabled?", ScreenSaver_enabled_p, 0);
    
    
    rb_gc_register_address(&hash_windowid_to_window);
    hash_windowid_to_window = rb_hash_new();
}
  
#ifdef HAVE_SDL_IMAGE_H
#include <SDL_image.h>

static VALUE mIMG;

/*
 * Document-module: SDL2::IMG
 *
 * This module provides the interface to SDL_image. You can load
 * many kinds of image files using this modules.
 * 
 * This module provides only initialization interface {SDL2::IMG.init}.
 * After calling init, you can load image files using {SDL2::Surface.load}.
 */

/*
 * @overload init(flags)
 *   Initialize SDL_image. 
 *   
 *   You can specify the supporting image formats by bitwise OR'd of the
 *   following constants.
 *
 *   * {SDL2::IMG::INIT_JPG}
 *   * {SDL2::IMG::INIT_PNG}
 *   * {SDL2::IMG::INIT_TIF}
 *   * {SDL2::IMG::INIT_WEBP}
 *
 *   You need to initialize SDL_image to check whether specified format
 *   is supported by your environment. If your environment does not
 *   support required support format, you have a {SDL2::Error} exception.
 *   
 *   @param [Integer] flags submodule bits
 *   @return [nil]
 *
 *   @raise [SDL2::Error] raised when initializing is failed.
 */
static VALUE IMG_s_init(VALUE self, VALUE f)
{
    int flags = NUM2INT(f);
    if (IMG_Init(flags) & flags != flags) 
        rb_raise(eSDL2Error, "Couldn't initialze SDL_image");
    return Qnil;
}

/*
 * @overload load(file) 
 *   Load file and create a new {SDL2::Surface}.
 *
 *   This method uses SDL_image. SDL_image supports following formats:
 *   BMP, CUR, GIF, ICO, JPG, LBP, PCX, PNG, PNM, TGA, TIF, XCF, XPM, and XV.
 *   
 *   @param [String] file the image file name to load a surface from
 *   @return [SDL2::Surface] Created surface
 *
 *   @raise [SDL2::Error] raised when you fail to load (for example,
 *     you have a wrong file name, or the file is broken)
 *
 *   @see SDL2::IMG.init
 *   @see SDL2::Renderer#load_texture
 */
static VALUE Surface_s_load(VALUE self, VALUE fname)
{
    SDL_Surface* surface = IMG_Load(StringValueCStr(fname));
    if (!surface) {
        SDL_SetError(IMG_GetError());
        SDL_ERROR();
    }
    return Surface_new(surface);
}

/*
 * @overload load_texture(file)
 *
 *   Load file and create a new {SDL2::Texture}.
 *
 *   This method uses SDL_image. SDL_image supports following formats:
 *   BMP, CUR, GIF, ICO, JPG, LBP, PCX, PNG, PNM, TGA, TIF, XCF, XPM, and XV.
 *
 *   @param [String] file the image file name to load a texture from
 *   @return [SDL2::Texture] Created texture
 *
 *   @raise [SDL2::Error] raised when you fail to load (for example,
 *     you have a wrong file name, or the file is broken)
 *
 *   @see SDL2::IMG.init
 *   @see SDL2::Surface.load
 */
static VALUE Renderer_load_texture(VALUE self, VALUE fname)
{
    SDL_Texture* texture = IMG_LoadTexture(Get_SDL_Renderer(self), StringValueCStr(fname));
    if (!texture) {
        SDL_SetError(IMG_GetError());
        SDL_ERROR();
    }
    return Texture_new(texture, Get_Renderer(self));
}

void rubysdl2_init_image(void)
{
    mIMG = rb_define_module_under(mSDL2, "IMG");
    rb_define_module_function(mIMG, "init", IMG_s_init, 1);

    rb_define_singleton_method(cSurface, "load", Surface_s_load, 1);
    rb_define_method(cRenderer, "load_texture", Renderer_load_texture, 1);
                     

    /* Initialize the JPEG loader */
    rb_define_const(mIMG, "INIT_JPG", INT2NUM(IMG_INIT_JPG));
    /* Initialize the PNG loader */
    rb_define_const(mIMG, "INIT_PNG", INT2NUM(IMG_INIT_PNG));
    /* Initialize the TIF loader */
    rb_define_const(mIMG, "INIT_TIF", INT2NUM(IMG_INIT_TIF));
    /* Initialize the WEBP loader */
    rb_define_const(mIMG, "INIT_WEBP", INT2NUM(IMG_INIT_WEBP));
}

#else /* HAVE_SDL_IMAGE_H */
void rubysdl2_init_image(void)
{
}
#endif
