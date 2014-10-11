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

#define TTF_ATTRIBUTE(attr, capitalized_attr, c2ruby, ruby2c)           \
    static VALUE TTF_##attr(VALUE self)                                 \
    {                                                                   \
        return c2ruby(TTF_Get##capitalized_attr(Get_TTF_Font(self)));   \
    }                                                                   \
    static VALUE TTF_set_##attr(VALUE self, VALUE val)                  \
    {                                                                   \
        TTF_Set##capitalized_attr(Get_TTF_Font(self), ruby2c(val));     \
            return Qnil;                                                \
    }

#define TTF_ATTRIBUTE_INT(attr, capitalized_attr)               \
    TTF_ATTRIBUTE(attr, capitalized_attr, INT2NUM, NUM2INT)

#define TTF_ATTR_READER(attr, capitalized_attr, c2ruby)                 \
    static VALUE TTF_##attr(VALUE self)                                 \
    {                                                                   \
        return c2ruby(TTF_Font##capitalized_attr(Get_TTF_Font(self)));  \
    }

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

static VALUE TTF_destroy(VALUE self)
{
    TTF* f = Get_TTF(self);
    if (f->font)
        TTF_CloseFont(f->font);
    f->font = NULL;
    return Qnil;
}

TTF_ATTRIBUTE_INT(style, FontStyle);
TTF_ATTRIBUTE_INT(outline, FontOutline);
TTF_ATTRIBUTE_INT(hinting, FontHinting);
TTF_ATTRIBUTE(kerning, FontKerning, INT2BOOL, RTEST);
TTF_ATTR_READER(height, Height, INT2FIX);
TTF_ATTR_READER(ascent, Ascent, INT2FIX);
TTF_ATTR_READER(descent, Descent, INT2FIX);
TTF_ATTR_READER(line_skip, LineSkip, INT2FIX);
TTF_ATTR_READER(num_faces, Faces, LONG2NUM);
TTF_ATTR_READER(face_is_fixed_width_p, FaceIsFixedWidth, INT2BOOL);
TTF_ATTR_READER(face_family_name, FaceFamilyName, utf8str_new_cstr);
TTF_ATTR_READER(face_style_name, FaceStyleName, utf8str_new_cstr);

static VALUE TTF_size_text(VALUE self, VALUE text)
{
    int w, h;
    text = rb_str_export_to_enc(text, rb_utf8_encoding());
    HANDLE_TTF_ERROR(TTF_SizeUTF8(Get_TTF_Font(self), StringValueCStr(text), &w, &h));
    return rb_ary_new3(2, INT2NUM(w), INT2NUM(h));
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
    rb_define_method(cTTF, "destroy", TTF_destroy, 0);
    
#define DEFINE_TTF_ATTRIBUTE(attr) do {                         \
        rb_define_method(cTTF, #attr, TTF_##attr, 0);           \
        rb_define_method(cTTF, #attr "=", TTF_set_##attr, 1);   \
    } while (0)
    
    DEFINE_TTF_ATTRIBUTE(style);
    DEFINE_TTF_ATTRIBUTE(outline);
    DEFINE_TTF_ATTRIBUTE(hinting);
    DEFINE_TTF_ATTRIBUTE(kerning);
    
#define DEFINE_TTF_ATTR_READER(attr)                    \
    rb_define_method(cTTF, #attr, TTF_##attr, 0)

    DEFINE_TTF_ATTR_READER(height);
    DEFINE_TTF_ATTR_READER(ascent);
    DEFINE_TTF_ATTR_READER(descent);
    DEFINE_TTF_ATTR_READER(line_skip);
    DEFINE_TTF_ATTR_READER(num_faces);
    rb_define_method(cTTF, "face_is_fixed_width?", TTF_face_is_fixed_width_p, 0);
    DEFINE_TTF_ATTR_READER(face_family_name);
    DEFINE_TTF_ATTR_READER(face_style_name);

    rb_define_method(cTTF, "size_text", TTF_size_text, 1);
    rb_define_method(cTTF, "render_solid", TTF_render_solid, 2);
    rb_define_method(cTTF, "render_shaded", TTF_render_shaded, 3);
    rb_define_method(cTTF, "render_blended", TTF_render_blended, 2);

#define DEFINE_TTF_CONST(name)                  \
    rb_define_const(cTTF, #name, INT2NUM((TTF_##name)))
    DEFINE_TTF_CONST(STYLE_NORMAL);
    DEFINE_TTF_CONST(STYLE_BOLD);
    DEFINE_TTF_CONST(STYLE_ITALIC);
    DEFINE_TTF_CONST(STYLE_UNDERLINE);
    DEFINE_TTF_CONST(STYLE_STRIKETHROUGH);

    DEFINE_TTF_CONST(HINTING_NORMAL);
    DEFINE_TTF_CONST(HINTING_LIGHT);
    DEFINE_TTF_CONST(HINTING_MONO);
    DEFINE_TTF_CONST(HINTING_NONE);
}

#else /* HAVE_SDL_TTF_H */
void rubysdl2_init_ttf(void)
{
}
#endif /* HAVE_SDL_TTF_H */

