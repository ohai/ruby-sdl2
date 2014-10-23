#include "rubysdl2_internal.h"
#include <SDL_video.h>
#include <SDL_version.h>
#include <SDL_render.h>
#include <SDL_messagebox.h>
#include <SDL_endian.h>
#include <ruby/encoding.h>

static VALUE cWindow;
static VALUE cDisplay;
static VALUE cDisplayMode;
static VALUE cRenderer;
static VALUE cTexture;
static VALUE cRect;
static VALUE cPoint;
static VALUE cSurface;
static VALUE cRendererInfo;
static VALUE cPixelFormat; /* NOTE: This is related to SDL_PixelFormatEnum, not SDL_PixelFormat */
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
    if (s->surface && rubysdl2_is_active())
        SDL_FreeSurface(s->surface);
    free(s);
}

VALUE Surface_new(SDL_Surface* surface)
{
    Surface* s = ALLOC(Surface);
    s->surface = surface;
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

static VALUE SDL2_s_video_drivers(VALUE self)
{
    int num_drivers = HANDLE_ERROR(SDL_GetNumVideoDrivers());
    int i;
    VALUE drivers = rb_ary_new();
    for (i=0; i<num_drivers; ++i)
        rb_ary_push(drivers, rb_usascii_str_new_cstr(SDL_GetVideoDriver(i)));
    return drivers;
}

static VALUE SDL2_s_current_video_driver(VALUE self)
{
    const char* name = SDL_GetCurrentVideoDriver();
    if (name)
        return utf8str_new_cstr(name);
    else
        return Qnil;
}

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
 * If you want to use graphical application using Ruby/SDL, first you need to
 * create a window.
 * 
 * All of methods/class methods are available only after initializing video
 * subsystem by {SDL2.init}.
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

static VALUE Window_s_all_windows(VALUE self)
{
    return rb_hash_dup(hash_windowid_to_window);
}

static VALUE Window_s_find_by_id(VALUE self, VALUE id)
{
    return rb_hash_aref(hash_windowid_to_window, id);
}

VALUE find_window_by_id(Uint32 id)
{
    return Window_s_find_by_id(Qnil, UINT2NUM(id));
}

static VALUE Window_destroy(VALUE self)
{
    Window_destroy_internal(Get_Window(self));
    return Qnil;
}

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

static VALUE Window_renderer(VALUE self)
{
    return rb_iv_get(self, "renderer");
}

static VALUE Window_window_id(VALUE self)
{
    return UINT2NUM(SDL_GetWindowID(Get_SDL_Window(self)));
}

static VALUE Window_display_mode(VALUE self)
{
    SDL_DisplayMode mode;
    HANDLE_ERROR(SDL_GetWindowDisplayMode(Get_SDL_Window(self), &mode));
    return DisplayMode_new(&mode);
}

static VALUE Window_display(VALUE self)
{
    int display_index = HANDLE_ERROR(SDL_GetWindowDisplayIndex(Get_SDL_Window(self)));
    return Display_new(display_index);
}

static VALUE Window_brightness(VALUE self)
{
    return DBL2NUM(SDL_GetWindowBrightness(Get_SDL_Window(self)));
}

static VALUE Window_set_brightness(VALUE self, VALUE brightness)
{
    HANDLE_ERROR(SDL_SetWindowBrightness(Get_SDL_Window(self), NUM2DBL(brightness)));
    return brightness;
}

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

static VALUE Window_gamma_ramp(VALUE self)
{
    Uint16 r[256], g[256], b[256];
    HANDLE_ERROR(SDL_GetWindowGammaRamp(Get_SDL_Window(self), r, g, b));
    return rb_ary_new3(3,
                       gamma_table_to_Array(r),
                       gamma_table_to_Array(g),
                       gamma_table_to_Array(b));
}

static VALUE Window_set_icon(VALUE self, VALUE icon)
{
    SDL_SetWindowIcon(Get_SDL_Window(self), Get_SDL_Surface(icon));
    return icon;
}

/* Window_set_gamma_ramp */

static VALUE Window_input_is_grabbed_p(VALUE self)
{
    return INT2BOOL(SDL_GetWindowGrab(Get_SDL_Window(self)));
}

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

static VALUE Window_maximum_size(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowMaximumSize, self);
}

static VALUE Window_set_maximum_size(VALUE self, VALUE max_size)
{
    return Window_set_int_int(SDL_SetWindowMaximumSize, self, max_size);
}

static VALUE Window_minimum_size(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowMinimumSize, self);
}

static VALUE Window_set_minimum_size(VALUE self, VALUE min_size)
{
    return Window_set_int_int(SDL_SetWindowMinimumSize, self, min_size);
}

static VALUE Window_position(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowPosition, self);
}

static VALUE Window_set_position(VALUE self, VALUE xy)
{
    return Window_set_int_int(SDL_SetWindowPosition, self, xy);
}

static VALUE Window_size(VALUE self)
{
    return Window_get_int_int(SDL_GetWindowSize, self);
}

static VALUE Window_set_size(VALUE self, VALUE size)
{
    return Window_set_int_int(SDL_SetWindowSize, self, size);
}

static VALUE Window_title(VALUE self)
{
    return utf8str_new_cstr(SDL_GetWindowTitle(Get_SDL_Window(self)));
}

static VALUE Window_bordered(VALUE self)
{
    return INT2BOOL(!(SDL_GetWindowFlags(Get_SDL_Window(self)) & SDL_WINDOW_BORDERLESS));
}

static VALUE Window_set_bordered(VALUE self, VALUE bordered)
{
    SDL_SetWindowBordered(Get_SDL_Window(self), RTEST(bordered));
    return bordered;
}

static VALUE Window_set_title(VALUE self, VALUE title)
{
    title = rb_str_export_to_enc(title, rb_utf8_encoding());
    SDL_SetWindowTitle(Get_SDL_Window(self), StringValueCStr(title));
    return Qnil;
}

#define SIMPLE_WINDOW_METHOD(SDL_name, name)                            \
    static VALUE Window_##name(VALUE self)                              \
    {                                                                   \
        SDL_##SDL_name##Window(Get_SDL_Window(self)); return Qnil;      \
    }

SIMPLE_WINDOW_METHOD(Show, show);
SIMPLE_WINDOW_METHOD(Hide, hide);
SIMPLE_WINDOW_METHOD(Maximize, maximize);
SIMPLE_WINDOW_METHOD(Minimize, minimize);
SIMPLE_WINDOW_METHOD(Raise, raise);
SIMPLE_WINDOW_METHOD(Restore, restore);

static VALUE Window_fullscreen_mode(VALUE self)
{
    Uint32 flags = SDL_GetWindowFlags(Get_SDL_Window(self));
    return UINT2NUM(flags & (SDL_WINDOW_FULLSCREEN|SDL_WINDOW_FULLSCREEN_DESKTOP));
}

static VALUE Window_set_fullscreen_mode(VALUE self, VALUE flags)
{
    HANDLE_ERROR(SDL_SetWindowFullscreen(Get_SDL_Window(self), NUM2UINT(flags)));
    return flags;
}

static VALUE Window_inspect(VALUE self)
{
    Window* w = Get_Window(self);
    if (w->window)
        return rb_sprintf("<%s:%p window_id=%d>",
                          rb_obj_classname(self), (void*)self, SDL_GetWindowID(w->window));
    else
        return rb_sprintf("<%s:%p (destroyed)>", rb_obj_classname(self), (void*)self);
}

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

static VALUE Display_s_displays(VALUE self)
{
    int i;
    int num_displays = SDL_GetNumVideoDisplays();
    VALUE displays = rb_ary_new2(num_displays);
    for (i=0; i<num_displays; ++i)
        rb_ary_push(displays, Display_new(i));
    return displays;
}

static int Display_index_int(VALUE display)
{
    return NUM2INT(rb_iv_get(display, "@index"));
}

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

static VALUE Display_current_mode(VALUE self)
{
    SDL_DisplayMode mode;
    HANDLE_ERROR(SDL_GetCurrentDisplayMode(Display_index_int(self), &mode));
    return DisplayMode_new(&mode);
}

static VALUE Display_desktop_mode(VALUE self)
{
    SDL_DisplayMode mode;
    HANDLE_ERROR(SDL_GetDesktopDisplayMode(Display_index_int(self), &mode));
    return DisplayMode_new(&mode);
}

static VALUE Display_closest_mode(VALUE self, VALUE mode)
{
    SDL_DisplayMode closest;
    if (!SDL_GetClosestDisplayMode(Display_index_int(self), Get_SDL_DisplayMode(mode),
                                   &closest))
        SDL_ERROR();
    return DisplayMode_new(&closest);
}

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

static VALUE DisplayMode_initialize(VALUE self, VALUE format, VALUE w, VALUE h,
                                    VALUE refresh_rate)
{
    SDL_DisplayMode* mode = Get_SDL_DisplayMode(self);
    mode->format = uint32_for_format(format);
    mode->w = NUM2INT(w); mode->h = NUM2INT(h);
    mode->refresh_rate = NUM2INT(refresh_rate);
    return Qnil;
}

static VALUE DisplayMode_inspect(VALUE self)
{
    SDL_DisplayMode* mode = Get_SDL_DisplayMode(self);
    return rb_sprintf("<%s: format=%s w=%d h=%d refresh_rate=%d>",
                      rb_obj_classname(self), SDL_GetPixelFormatName(mode->format),
                      mode->w, mode->h, mode->refresh_rate);
                      
}

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

static VALUE Renderer_destroy(VALUE self)
{
    Renderer_destroy_internal(Get_Renderer(self));
    return Qnil;
}

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

static VALUE Renderer_copy(VALUE self, VALUE texture, VALUE srcrect, VALUE dstrect)
{
    HANDLE_ERROR(SDL_RenderCopy(Get_SDL_Renderer(self),
                                Get_SDL_Texture(texture),
                                Get_SDL_Rect_or_NULL(srcrect),
                                Get_SDL_Rect_or_NULL(dstrect)));
    return Qnil;
}

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

static VALUE Renderer_present(VALUE self)
{
    SDL_RenderPresent(Get_SDL_Renderer(self));
    return Qnil;
}

static VALUE Renderer_clear(VALUE self)
{
    HANDLE_ERROR(SDL_RenderClear(Get_SDL_Renderer(self)));
    return Qnil;
}

static VALUE Renderer_draw_color(VALUE self)
{
    Uint8 r, g, b, a;
    HANDLE_ERROR(SDL_GetRenderDrawColor(Get_SDL_Renderer(self), &r, &g, &b, &a));
    return rb_ary_new3(4, INT2FIX(r), INT2FIX(g), INT2FIX(b), INT2FIX(a));
}

static VALUE Renderer_set_draw_color(VALUE self, VALUE rgba)
{
    SDL_Color color = Array_to_SDL_Color(rgba);
    
    HANDLE_ERROR(SDL_SetRenderDrawColor(Get_SDL_Renderer(self),
                                        color.r, color.g, color.b, color.a));
                                        
    return Qnil;
}


static VALUE Renderer_draw_line(VALUE self, VALUE x1, VALUE y1, VALUE x2, VALUE y2)
{
    HANDLE_ERROR(SDL_RenderDrawLine(Get_SDL_Renderer(self),
                                    NUM2INT(x1), NUM2INT(y1), NUM2INT(x2), NUM2INT(y2)));
    return Qnil;
}

static VALUE Renderer_draw_point(VALUE self, VALUE x, VALUE y)
{
    HANDLE_ERROR(SDL_RenderDrawPoint(Get_SDL_Renderer(self), NUM2INT(x), NUM2INT(y)));
    return Qnil;
}

static VALUE Renderer_draw_rect(VALUE self, VALUE rect)
{
    HANDLE_ERROR(SDL_RenderDrawRect(Get_SDL_Renderer(self), Get_SDL_Rect(rect)));
    return Qnil;
}

static VALUE Renderer_fill_rect(VALUE self, VALUE rect)
{
    HANDLE_ERROR(SDL_RenderFillRect(Get_SDL_Renderer(self), Get_SDL_Rect(rect)));
    return Qnil;
}

static VALUE Renderer_info(VALUE self)
{
    SDL_RendererInfo info;
    HANDLE_ERROR(SDL_GetRendererInfo(Get_SDL_Renderer(self), &info));
    return RendererInfo_new(&info);
}

static VALUE Renderer_draw_blend_mode(VALUE self)
{
    SDL_BlendMode mode;
    HANDLE_ERROR(SDL_GetRenderDrawBlendMode(Get_SDL_Renderer(self), &mode));
    return INT2FIX(mode);
}

static VALUE Renderer_set_draw_blend_mode(VALUE self, VALUE mode)
{
    HANDLE_ERROR(SDL_SetRenderDrawBlendMode(Get_SDL_Renderer(self), NUM2INT(mode)));
    return mode;
}

static VALUE Renderer_clip_rect(VALUE self)
{
    VALUE rect = rb_obj_alloc(cRect);
    SDL_RenderGetClipRect(Get_SDL_Renderer(self), Get_SDL_Rect(rect));
    return rect;
}

#if SDL_VERSION_ATLEAST(2,0,4)
static VALUE Render_clip_enabled_p(VALUE self)
{
    return INT2BOOL(SDL_RenderIsClipEnabled(Get_SDL_Renderer(self)));
}
#endif

static VALUE Renderer_logical_size(VALUE self)
{
    int w, h;
    SDL_RenderGetLogicalSize(Get_SDL_Renderer(self), &w, &h);
    return rb_ary_new3(2, INT2FIX(w), INT2FIX(h));
}

static VALUE Renderer_scale(VALUE self)
{
    float scaleX, scaleY;
    SDL_RenderGetScale(Get_SDL_Renderer(self), &scaleX, &scaleY);
    return rb_ary_new3(2, DBL2NUM(scaleX), DBL2NUM(scaleY));
}

static VALUE Renderer_viewport(VALUE self)
{
    VALUE rect = rb_obj_alloc(cRect);
    SDL_RenderGetViewport(Get_SDL_Renderer(self), Get_SDL_Rect(rect));
    return rect;
}

static VALUE Renderer_support_render_target_p(VALUE self)
{
    return INT2BOOL(SDL_RenderTargetSupported(Get_SDL_Renderer(self)));
}

static VALUE Renderer_output_size(VALUE self)
{
    int w, h;
    HANDLE_ERROR(SDL_GetRendererOutputSize(Get_SDL_Renderer(self), &w, &h));
    return rb_ary_new3(2, INT2FIX(w), INT2FIX(h));
}

static VALUE Renderer_set_render_target(VALUE self, VALUE target)
{
    HANDLE_ERROR(SDL_SetRenderTarget(Get_SDL_Renderer(self),
                                     (target == Qnil) ? NULL : Get_SDL_Texture(target)));
    rb_iv_set(self, "render_target", target);
    return Qnil;
}

static VALUE Renderer_render_target(VALUE self)
{
    return rb_iv_get(self, "render_target");
}

static VALUE Renderer_reset_render_target(VALUE self)
{
    return Renderer_set_render_target(self, Qnil);
}

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

static VALUE Texture_destroy(VALUE self)
{
    Texture_destroy_internal(Get_Texture(self));
    return Qnil;
}

static VALUE Texture_blend_mode(VALUE self)
{
    SDL_BlendMode mode;
    HANDLE_ERROR(SDL_GetTextureBlendMode(Get_SDL_Texture(self), &mode));
    return INT2FIX(mode);
}

static VALUE Texture_set_blend_mode(VALUE self, VALUE mode)
{
    HANDLE_ERROR(SDL_SetTextureBlendMode(Get_SDL_Texture(self), NUM2INT(mode)));
    return mode;
}

static VALUE Texture_alpha_mod(VALUE self)
{
    Uint8 alpha;
    HANDLE_ERROR(SDL_GetTextureAlphaMod(Get_SDL_Texture(self), &alpha));
    return INT2FIX(alpha);
}

static VALUE Texture_set_alpha_mod(VALUE self, VALUE alpha)
{
    HANDLE_ERROR(SDL_SetTextureAlphaMod(Get_SDL_Texture(self), NUM2UCHAR(alpha)));
    return alpha;
}

static VALUE Texture_color_mod(VALUE self)
{
    Uint8 r, g, b;
    HANDLE_ERROR(SDL_GetTextureColorMod(Get_SDL_Texture(self), &r, &g, &b));
    return rb_ary_new3(3, INT2FIX(r), INT2FIX(g), INT2FIX(b));
}

static VALUE Texture_set_color_mod(VALUE self, VALUE rgb)
{
    SDL_Color color = Array_to_SDL_Color(rgb);
    HANDLE_ERROR(SDL_SetTextureColorMod(Get_SDL_Texture(self),
                                        color.r, color.g, color.b));
    return Qnil;
}

static VALUE Texture_format(VALUE self)
{
    Uint32 format;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), &format, NULL, NULL, NULL));
    return PixelFormat_new(format);
}

static VALUE Texture_access_pattern(VALUE self)
{
    int access;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), NULL, &access, NULL, NULL));
    return INT2FIX(access);
}

static VALUE Texture_w(VALUE self)
{
    int w;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), NULL, NULL, &w, NULL));
    return INT2FIX(w);
}

static VALUE Texture_h(VALUE self)
{
    int h;
    HANDLE_ERROR(SDL_QueryTexture(Get_SDL_Texture(self), NULL, NULL, NULL, &h));
    return INT2FIX(h);
}

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

static VALUE Texture_debug_info(VALUE self)
{
    Texture* t = Get_Texture(self);
    VALUE info = rb_hash_new();
    rb_hash_aset(info, rb_str_new2("destroy?"), INT2BOOL(t->texture == NULL));
    rb_hash_aset(info, rb_str_new2("refcount"), INT2NUM(t->refcount));
    return info;
}

static VALUE Surface_s_load_bmp(VALUE self, VALUE fname)
{
    SDL_Surface* surface = SDL_LoadBMP(StringValueCStr(fname));

    if (surface == NULL)
        HANDLE_ERROR(-1);

    return Surface_new(surface);
}

static VALUE Surface_destroy(VALUE self)
{
    Surface* s = Get_Surface(self);
    if (s->surface)
        SDL_FreeSurface(s->surface);
    s->surface = NULL;
    return Qnil;
}

static VALUE Surface_blend_mode(VALUE self)
{
    SDL_BlendMode mode;
    HANDLE_ERROR(SDL_GetSurfaceBlendMode(Get_SDL_Surface(self), &mode));
    return INT2FIX(mode);
}

static VALUE Surface_set_blend_mode(VALUE self, VALUE mode)
{
    HANDLE_ERROR(SDL_SetSurfaceBlendMode(Get_SDL_Surface(self), NUM2INT(mode)));
    return mode;
}

static VALUE Surface_must_lock_p(VALUE self)
{
    return INT2BOOL(SDL_MUSTLOCK(Get_SDL_Surface(self)));
}

static VALUE Surface_lock(VALUE self)
{
    HANDLE_ERROR(SDL_LockSurface(Get_SDL_Surface(self)));
    return Qnil;
}

static VALUE Surface_unlock(VALUE self)
{
    SDL_UnlockSurface(Get_SDL_Surface(self));
    return Qnil;
}

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

static VALUE Surface_unset_color_key(VALUE self)
{
    HANDLE_ERROR(SDL_SetColorKey(Get_SDL_Surface(self), SDL_FALSE, 0));
    return Qnil;
}

static VALUE Surface_set_color_key(VALUE self, VALUE key)
{
    SDL_Surface* surface = Get_SDL_Surface(self);
    if (key == Qnil)
        return Surface_unset_color_key(self);
    
    HANDLE_ERROR(SDL_SetColorKey(surface, SDL_TRUE, pixel_value(key, surface->format)));
    
    return key;
}

static VALUE Surface_color_key(VALUE self)
{
    Uint32 key;
    if (SDL_GetColorKey(Get_SDL_Surface(self), &key) < 0)
        return Qnil;
    else
        return UINT2NUM(key);
}

static VALUE Surface_w(VALUE self)
{
    return INT2NUM(Get_SDL_Surface(self)->w);
}

static VALUE Surface_h(VALUE self)
{
    return INT2NUM(Get_SDL_Surface(self)->h);
}


static VALUE Surface_s_blit(VALUE self, VALUE src, VALUE srcrect, VALUE dst, VALUE dstrect)
{
    HANDLE_ERROR(SDL_BlitSurface(Get_SDL_Surface(src),
                                 Get_SDL_Rect_or_NULL(srcrect),
                                 Get_SDL_Surface(dst),
                                 Get_SDL_Rect_or_NULL(dstrect)));
    return Qnil;
}

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
 * @!method x
 *   Return x
 *
 * @!method x=(val)
 *   Set x to val
 *
 * @!method y
 *   Return y
 *   
 * @!method y=(val)
 *   Set y to val
 */
static VALUE Point_s_allocate(VALUE klass)
{
    SDL_Point* point;
    return Data_Make_Struct(klass, SDL_Point, 0, free, point);
}

static VALUE Point_initialize(int argc, VALUE* argv, VALUE self)
{
    VALUE x, y;
    SDL_Point* point = Get_SDL_Point(self);
    rb_scan_args(argc, argv, "02", &x, &y);
    point->x = (x == Qnil) ? 0 : NUM2INT(x);
    point->y = (y == Qnil) ? 0 : NUM2INT(y);
    return Qnil;
}

static VALUE Point_inspect(VALUE self)
{
    SDL_Point* point = Get_SDL_Point(self);
    return rb_sprintf("<SDL2::Point x=%d y=%d>", point->x, point->y);
}

FIELD_ACCESSOR(Point, SDL_Point, x);
FIELD_ACCESSOR(Point, SDL_Point, y);

static VALUE PixelForamt_initialize(VALUE self, VALUE format)
{
    rb_iv_set(self, "@format", format);
    return Qnil;
}

static VALUE PixelFormat_type(VALUE self)
{
    return UINT2NUM(SDL_PIXELTYPE(rb_iv_get(self, "@format")));
}

#define PIXELFORMAT_ATTR_READER(field, extractor, c2ruby)               \
    static VALUE PixelFormat_##field(VALUE self)                        \
    {                                                                   \
        return c2ruby(extractor(NUM2UINT(rb_iv_get(self, "@format")))); \
    }

PIXELFORMAT_ATTR_READER(name, SDL_GetPixelFormatName, utf8str_new_cstr);
PIXELFORMAT_ATTR_READER(order,  SDL_PIXELORDER, UINT2NUM);
PIXELFORMAT_ATTR_READER(layout,  SDL_PIXELLAYOUT, UINT2NUM);
PIXELFORMAT_ATTR_READER(bits_per_pixel,  SDL_BITSPERPIXEL, INT2NUM);
PIXELFORMAT_ATTR_READER(bytes_per_pixel,  SDL_BYTESPERPIXEL, INT2NUM);
PIXELFORMAT_ATTR_READER(indexed_p,  SDL_ISPIXELFORMAT_INDEXED, INT2BOOL);
PIXELFORMAT_ATTR_READER(alpha_p,  SDL_ISPIXELFORMAT_ALPHA, INT2BOOL);
PIXELFORMAT_ATTR_READER(fourcc_p,  SDL_ISPIXELFORMAT_FOURCC, INT2BOOL);

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

static VALUE ScreenSaver_enable(VALUE self)
{
    SDL_EnableScreenSaver();
    return Qnil;
}

static VALUE ScreenSaver_disable(VALUE self)
{
    SDL_DisableScreenSaver();
    return Qnil;
}

static VALUE ScreenSaver_enabled_p(VALUE self)
{
    return INT2BOOL(SDL_IsScreenSaverEnabled());
}

#define DEFINE_C_ACCESSOR(classname, classvar, field)               \
    rb_define_method(classvar, #field, classname##_##field, 0);         \
    rb_define_method(classvar, #field "=", classname##_set_##field, 1);
    
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
    rb_define_const(cWindow, "POS_CENTERED", INT2NUM(SDL_WINDOWPOS_CENTERED));
    rb_define_const(cWindow, "POS_UNDEFINED", INT2NUM(SDL_WINDOWPOS_UNDEFINED));
#define DEFINE_SDL_WINDOW_FLAGS_CONST(n)                        \
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
    rb_define_private_method(cDisplayMode, "initialize", DisplayMode_initialize, 4);
    rb_define_method(cDisplayMode, "inspect", DisplayMode_inspect, 0);

    
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
#if SDL_VERSION_ATLEAST(2,0,4)
    rb_define_method(cRenderer, "clip_enabled?", Render_clip_enabled_p, 0);
#endif
    rb_define_method(cRenderer, "logical_size", Renderer_logical_size, 0);
    rb_define_method(cRenderer, "scale", Renderer_scale, 0);
    rb_define_method(cRenderer, "viewport", Renderer_viewport, 0);
    rb_define_method(cRenderer, "support_render_target?", Renderer_support_render_target_p, 0);
    rb_define_method(cRenderer, "output_size", Renderer_output_size, 0);
    rb_define_method(cRenderer, "render_target", Renderer_render_target, 0);
    rb_define_method(cRenderer, "render_target=", Renderer_set_render_target, 1);
    rb_define_method(cRenderer, "reset_render_target", Renderer_reset_render_target, 0);
    
    rb_define_method(cRenderer, "info", Renderer_info, 0);
#define DEFINE_SDL_RENDERER_FLAGS_CONST(n)                      \
    rb_define_const(cRenderer, #n, UINT2NUM(SDL_RENDERER_##n))
    DEFINE_SDL_RENDERER_FLAGS_CONST(SOFTWARE);
    DEFINE_SDL_RENDERER_FLAGS_CONST(ACCELERATED);
#ifdef SDL_RENDERER_PRESENTVSYNC
    DEFINE_SDL_RENDERER_FLAGS_CONST(PRESENTVSYNC);
#endif
    DEFINE_SDL_RENDERER_FLAGS_CONST(TARGETTEXTURE);
#define DEFINE_SDL_FLIP_CONST(t)                                        \
    rb_define_const(cRenderer, "FLIP_" #t, INT2FIX(SDL_FLIP_##t))
    DEFINE_SDL_FLIP_CONST(NONE);
    DEFINE_SDL_FLIP_CONST(HORIZONTAL);
    DEFINE_SDL_FLIP_CONST(VERTICAL);
#define DEFINE_BLENDMODE_CONST(t)                                       \
    rb_define_const(mSDL2, "BLENDMODE_" #t, INT2FIX(SDL_BLENDMODE_##t))
    DEFINE_BLENDMODE_CONST(NONE);
    DEFINE_BLENDMODE_CONST(BLEND);
    DEFINE_BLENDMODE_CONST(ADD);
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
#define DEFINE_TEXTUREAH_ACCESS_CONST(t)                                \
    rb_define_const(cTexture, "ACCESS_" #t, INT2NUM(SDL_TEXTUREACCESS_##t))
    DEFINE_TEXTUREAH_ACCESS_CONST(STATIC);
    DEFINE_TEXTUREAH_ACCESS_CONST(STREAMING);
    DEFINE_TEXTUREAH_ACCESS_CONST(TARGET);

    
    cSurface = rb_define_class_under(mSDL2, "Surface", rb_cObject);
    
    rb_undef_alloc_func(cSurface);
    rb_define_singleton_method(cSurface, "load_bmp", Surface_s_load_bmp, 1);
    rb_define_singleton_method(cSurface, "blit", Surface_s_blit, 4);
    rb_define_singleton_method(cSurface, "new", Surface_s_new, -1);
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
    rb_define_private_method(cPoint, "initialize", Point_initialize, -1);
    rb_define_alias(rb_singleton_class(cPoint), "[]", "new");
    rb_define_method(cPoint, "inspect", Point_inspect, 0);
    DEFINE_C_ACCESSOR(Point, cPoint, x);
    DEFINE_C_ACCESSOR(Point, cPoint, y);

    
    cRendererInfo = rb_define_class_under(cRenderer, "Info", rb_cObject);
    define_attr_readers(cRendererInfo, "name", "flags", "texture_formats",
                        "max_texture_width", "max_texture_height", NULL);
    
    
    cPixelFormat = rb_define_class_under(mSDL2, "PixelFormat", rb_cObject);
    
    rb_define_private_method(cPixelFormat, "initialize", PixelForamt_initialize, 1);
    rb_define_attr(cPixelFormat, "format", 1, 0);
    rb_define_method(cPixelFormat, "name", PixelFormat_name, 0);
    rb_define_method(cPixelFormat, "inspect", PixelFormat_inspect, 0);
    rb_define_method(cPixelFormat, "type", PixelFormat_type, 0);
    rb_define_method(cPixelFormat, "order", PixelFormat_order, 0);
    rb_define_method(cPixelFormat, "layout", PixelFormat_layout, 0);
    rb_define_method(cPixelFormat, "bits_per_pixel", PixelFormat_bits_per_pixel, 0);
    rb_define_alias(cPixelFormat, "bps", "bits_per_pixel");
    rb_define_method(cPixelFormat, "bytes_per_pixel", PixelFormat_bytes_per_pixel, 0);
    rb_define_method(cPixelFormat, "indexed?", PixelFormat_indexed_p, 0);
    rb_define_method(cPixelFormat, "alpha?", PixelFormat_alpha_p, 0);
    rb_define_method(cPixelFormat, "fourcc?", PixelFormat_fourcc_p, 0);

    {
        VALUE formats = rb_ary_new();
        rb_define_const(cPixelFormat, "FORMATS", formats);
#define DEFINE_PIXELFORMAT_CONST(t)                                     \
        do {                                                            \
            VALUE format = PixelFormat_new(SDL_PIXELFORMAT_##t);  \
            rb_define_const(cPixelFormat, #t, format);                  \
            rb_ary_push(formats, format);                               \
        } while (0)
        DEFINE_PIXELFORMAT_CONST(UNKNOWN);
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

static VALUE IMG_s_init(VALUE self, VALUE f)
{
    int flags = NUM2INT(f);
    if (IMG_Init(flags) & flags != flags) 
        rb_raise(eSDL2Error, "Couldn't initialze SDL_image");
    return Qnil;
}

static VALUE Surface_s_load(VALUE self, VALUE fname)
{
    SDL_Surface* surface = IMG_Load(StringValueCStr(fname));
    if (!surface) {
        SDL_SetError(IMG_GetError());
        SDL_ERROR();
    }
    return Surface_new(surface);
}

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
                     
    
    rb_define_const(mIMG, "INIT_JPG", INT2NUM(IMG_INIT_JPG));
    rb_define_const(mIMG, "INIT_PNG", INT2NUM(IMG_INIT_PNG));
    rb_define_const(mIMG, "INIT_TIF", INT2NUM(IMG_INIT_TIF));
    rb_define_const(mIMG, "INIT_WEBP", INT2NUM(IMG_INIT_WEBP));
}

#else /* HAVE_SDL_IMAGE_H */
void rubysdl2_init_image(void)
{
}
#endif
