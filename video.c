#include "rubysdl2_internal.h"
#include <SDL_video.h>
#include <SDL_render.h>
#include <ruby/encoding.h>

static VALUE cWindow;
static VALUE cRenderer;
static VALUE cTexture;
static VALUE cRect;
static VALUE cSurface;

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

typedef struct Surface {
    SDL_Surface* surface;
} Surface;

static void Renderer_free(Renderer*);
static void Window_free(Window* w)
{
    int i;
    for (i=0; i<w->num_renderers; ++i) 
        Renderer_free(w->renderers[i]);
  
    if (w->window && rubysdl2_is_active())
        SDL_DestroyWindow(w->window);
  
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

static void Texture_free(Texture*);
static void Renderer_free(Renderer* r)
{
    int i;
    for (i=0; i<r->num_textures; ++i)
        Texture_free(r->textures[i]);
    if (r->renderer && rubysdl2_is_active()) {
        SDL_DestroyRenderer(r->renderer);
    }
    r->renderer = NULL;
    r->refcount--;
    if (r->refcount == 0) {
        free(r->textures);
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


static void Texture_free(Texture* t)
{
    if (t->texture && rubysdl2_is_active()) {
        SDL_DestroyTexture(t->texture);
    }
    t->texture = NULL;
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
    if (s->surface && rubysdl2_is_active())
        SDL_FreeSurface(s->surface);
    free(s);
}

static VALUE Surface_new(SDL_Surface* surface)
{
    Surface* s = ALLOC(Surface);
    s->surface = surface;
    return Data_Wrap_Struct(cSurface, 0, Surface_free, s);
}

DEFINE_WRAPPER(SDL_Surface, Surface, surface, cSurface, "SDL2::Surface");

DEFINE_GETTER(SDL_Rect, cRect, "SDL2::Rect");

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

static VALUE Window_create_renderer(VALUE self, VALUE index, VALUE flags)
{
    SDL_Renderer* sdl_renderer;
    VALUE renderer;
    sdl_renderer = SDL_CreateRenderer(Get_SDL_Window(self), NUM2INT(index), NUM2UINT(flags));
  
    if (sdl_renderer == NULL)
        HANDLE_ERROR(-1);
  
    renderer = Renderer_new(sdl_renderer, Get_Window(self));
    return renderer;
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

static VALUE Renderer_create_texture_from(VALUE self, VALUE surface)
{
    SDL_Texture* texture = SDL_CreateTextureFromSurface(Get_SDL_Renderer(self),
                                                        Get_SDL_Surface(surface));
    if (texture == NULL)
        SDL_ERROR();
  
    return Texture_new(texture, Get_Renderer(self));
}

static VALUE Renderer_copy(VALUE self, VALUE texture, VALUE srcrect, VALUE dstrect)
{
    HANDLE_ERROR(SDL_RenderCopy(Get_SDL_Renderer(self),
                                Get_SDL_Texture(texture),
                                srcrect == Qnil ? NULL : Get_SDL_Rect(srcrect),
                                dstrect == Qnil ? NULL : Get_SDL_Rect(dstrect)));
    return Qnil;
}

static VALUE Renderer_present(VALUE self)
{
    SDL_RenderPresent(Get_SDL_Renderer(self));
    return Qnil;
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

static VALUE Rect_s_allocate(VALUE klass)
{
    SDL_Rect* rect = ALLOC(SDL_Rect);
    rect->x = rect->y = rect->w = rect->h = 0;
  
    return Data_Wrap_Struct(cRect, 0, free, rect);
}

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

#define RECT_ACCESSOR(field)                                    \
    static VALUE Rect_##field(VALUE self)                       \
    {                                                           \
        SDL_Rect* r; Data_Get_Struct(self, SDL_Rect, r);        \
        return INT2NUM(r->field);                               \
    }                                                           \
    static VALUE Rect_set_##field(VALUE self, VALUE val)        \
    {                                                           \
        SDL_Rect* r; Data_Get_Struct(self, SDL_Rect, r);        \
        r->field = NUM2INT(val); return val;                    \
    }

RECT_ACCESSOR(x);
RECT_ACCESSOR(y);
RECT_ACCESSOR(w);
RECT_ACCESSOR(h);

void rubysdl2_init_video(void)
{
    cWindow = rb_define_class_under(mSDL2, "Window", rb_cObject);
    
    rb_undef_alloc_func(cWindow);
    rb_define_singleton_method(cWindow, "create", Window_s_create, 6);
    rb_define_method(cWindow, "destroy?", Window_destroy_p, 0);
    rb_define_method(cWindow, "create_renderer", Window_create_renderer, 2);
    rb_define_method(cWindow, "debug_info", Window_debug_info, 0);
    rb_define_const(cWindow, "OP_CENTERED", INT2NUM(SDL_WINDOWPOS_CENTERED));
    rb_define_const(cWindow, "OP_UNDEFINED", INT2NUM(SDL_WINDOWPOS_UNDEFINED));
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
#undef DEFINE_SDL_WINDOW_FLAGS_CONST

  
    cRenderer = rb_define_class_under(mSDL2, "Renderer", rb_cObject);
    
    rb_undef_alloc_func(cRenderer);
    rb_define_method(cRenderer, "destroy?", Renderer_destroy_p, 0);
    rb_define_method(cRenderer, "debug_info", Renderer_debug_info, 0);
    rb_define_method(cRenderer, "create_texture_from", Renderer_create_texture_from, 1);
    rb_define_method(cRenderer, "copy", Renderer_copy, 3);
    rb_define_method(cRenderer, "present", Renderer_present, 0);
#define DEFINE_SDL_RENDERER_FLAGS_CONST(n)                      \
    rb_define_const(cRenderer, #n, UINT2NUM(SDL_RENDERER_##n))
    DEFINE_SDL_RENDERER_FLAGS_CONST(SOFTWARE);
    DEFINE_SDL_RENDERER_FLAGS_CONST(ACCELERATED);
#ifdef SDL_RENDERER_PRESENTVSYNC
    DEFINE_SDL_RENDERER_FLAGS_CONST(PRESENTVSYNC);
#endif
    DEFINE_SDL_RENDERER_FLAGS_CONST(TARGETTEXTURE);
#undef DEFINE_SDL_RENDERER_FLAGS_CONST
  
  
    cTexture = rb_define_class_under(mSDL2, "Texture", rb_cObject);
    
    rb_undef_alloc_func(cTexture);
    rb_define_method(cTexture, "destroy?", Texture_destroy_p, 0);
    rb_define_method(cTexture, "debug_info", Texture_debug_info, 0);
  
    cSurface = rb_define_class_under(mSDL2, "Surface", rb_cObject);
    
    rb_undef_alloc_func(cSurface);
    rb_define_singleton_method(cSurface, "load_bmp", Surface_s_load_bmp, 1);
    rb_define_method(cSurface, "destroy?", Surface_destroy_p, 0);
    rb_define_method(cSurface, "destroy", Surface_destroy, 0);


    cRect = rb_define_class_under(mSDL2, "Rect", rb_cObject);

    rb_define_alloc_func(cRect, Rect_s_allocate);
    rb_define_private_method(cRect, "initialize", Rect_initialize, -1);
#define DEFINE_RECT_ACCESSOR(field)                             \
    rb_define_method(cRect, #field, Rect_##field, 0);           \
    rb_define_method(cRect, #field "=", Rect_set_##field, 1);

    DEFINE_RECT_ACCESSOR(x);
    DEFINE_RECT_ACCESSOR(y);
    DEFINE_RECT_ACCESSOR(w);
    DEFINE_RECT_ACCESSOR(h);
#undef DEFINE_RECT_ACCESSOR
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
