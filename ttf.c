#ifdef HAVE_SDL_TTF_H
#include "rubysdl2_internal.h"
#include <SDL_ttf.h>
#include <ruby/encoding.h>

VALUE cTTF;

#define TTF_ERROR() do { HANDLE_ERROR(SDL_SetError(TTF_GetError())); } while (0)
#define HANDLE_TTF_ERROR(code) \
    do { if ((code) < 0) { TTF_ERROR(); } } while (0)

typedef struct TTF {
    TTF_Font* font;
} TTF;

static void TTF_free(TTF* f)
{
    if (rubysdl2_is_active() && f->font)
        TTF_CloseFont(f->font);
    free(f);
}

static VALUE TTF_new(TTF_Font* font)
{
    TTF* f = ALLOC(TTF);
    f->font = font;
    return Data_Wrap_Struct(cTTF, 0, TTF_free, f);
}

DEFINE_WRAPPER(TTF_Font, TTF, font, cTTF, "SDL2::TTF");

static VALUE TTF_s_init(VALUE self)
{
    HANDLE_TTF_ERROR(TTF_Init());
    return Qnil;
}

static VALUE TTF_s_open(int argc, VALUE* argv, VALUE self)
{
    TTF_Font* font;
    VALUE fname, ptsize, index;
    rb_scan_args(argc, argv, "21", &fname, &ptsize, &index);

    font = TTF_OpenFontIndex(StringValueCStr(fname), NUM2INT(ptsize),
                             index == Qnil ? 0 : NUM2LONG(index));
    if (!font)
        TTF_ERROR();
    
    return TTF_new(font);
}

static SDL_Surface* render_solid(TTF_Font* font, const char* text, SDL_Color fg, SDL_Color bg)
{
    return TTF_RenderUTF8_Solid(font, text, fg);
}

static SDL_Surface* render_blended(TTF_Font* font, const char* text, SDL_Color fg, SDL_Color bg)
{
    return TTF_RenderUTF8_Blended(font, text, fg);
}

static VALUE render(SDL_Surface* (*renderer)(TTF_Font*, const char*, SDL_Color, SDL_Color),
                    VALUE font, VALUE text, VALUE fg, VALUE bg)
{
    SDL_Surface* surface;
    text = rb_str_export_to_enc(text, rb_utf8_encoding());
    surface = renderer(Get_TTF_Font(font), StringValueCStr(text),
                       Array_to_SDL_Color(fg), Array_to_SDL_Color(bg));
    if (!surface)
        TTF_ERROR();

    return Surface_new(surface);
}

static VALUE TTF_render_solid(VALUE self, VALUE text, VALUE fg)
{
    return render(render_solid, self, text, fg, Qnil);
}

static VALUE TTF_render_shaded(VALUE self, VALUE text, VALUE fg, VALUE bg)
{
    return render(TTF_RenderUTF8_Shaded, self, text, fg, bg);
}

static VALUE TTF_render_blended(VALUE self, VALUE text, VALUE fg)
{
    return render(render_blended, self, text, fg, Qnil);
}

void rubysdl2_init_ttf(void)
{
    cTTF = rb_define_class_under(mSDL2, "TTF", rb_cObject);
    rb_undef_alloc_func(cTTF);

    rb_define_singleton_method(cTTF, "init", TTF_s_init, 0);
    rb_define_singleton_method(cTTF, "open", TTF_s_open, -1);
    rb_define_method(cTTF, "destroy?", TTF_destroy_p, 0);
    rb_define_method(cTTF, "render_solid", TTF_render_solid, 2);
    rb_define_method(cTTF, "render_shaded", TTF_render_shaded, 3);
    rb_define_method(cTTF, "render_blended", TTF_render_blended, 2);

}

#else /* HAVE_SDL_TTF_H */
void rubysdl2_init_ttf(void)
{
}
#endif /* HAVE_SDL_TTF_H */

