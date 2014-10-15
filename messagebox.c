#include "rubysdl2_internal.h"
#include <SDL_messagebox.h>

static VALUE sym_flags, sym_window, sym_title, sym_message, sym_buttons, sym_color_scheme,
    sym_id, sym_text, sym_bg, sym_button_border, sym_button_bg, sym_button_selected;

static inline SDL_Window* Get_SDL_Window_or_NULL(VALUE win)
{
    if (win == Qnil)
        return NULL;
    else
        return Get_SDL_Window(win);
}

static VALUE SDL2_s_show_simple_message_box(VALUE self, VALUE flag, VALUE title,
                                            VALUE message, VALUE parent)
{
    title = rb_str_export_to_utf8(title);
    message = rb_str_export_to_utf8(message);
    HANDLE_ERROR(SDL_ShowSimpleMessageBox(NUM2UINT(flag),
                                          StringValueCStr(title),
                                          StringValueCStr(message),
                                          Get_SDL_Window_or_NULL(parent)));
    return Qnil;
}

static void set_color_scheme(VALUE colors, VALUE sym, SDL_MessageBoxColor* color)
{
    VALUE c = rb_hash_aref(colors, sym);
    Check_Type(c, T_ARRAY);
    color->r = NUM2UCHAR(rb_ary_entry(c, 0));
    color->g = NUM2UCHAR(rb_ary_entry(c, 1));
    color->b = NUM2UCHAR(rb_ary_entry(c, 2));
}

static VALUE SDL2_s_show_message_box(VALUE self, VALUE params)
{
    SDL_MessageBoxData mb_data;
    VALUE title, message, texts, buttons, color_scheme;
    long num_buttons;
    int i;
    SDL_MessageBoxButtonData* button_data;
    SDL_MessageBoxColorScheme scheme;
    int buttonid;

    Check_Type(params, T_HASH);
    mb_data.flags = NUM2INT(rb_hash_aref(params, sym_flags));
    mb_data.window = Get_SDL_Window_or_NULL(rb_hash_aref(params, sym_window));
    title = rb_str_export_to_utf8(rb_hash_aref(params, sym_title));
    mb_data.title = StringValueCStr(title);
    message = rb_str_export_to_utf8(rb_hash_aref(params, sym_message));
    mb_data.message = StringValueCStr(message);
    
    buttons = rb_hash_aref(params, sym_buttons);
    Check_Type(buttons, T_ARRAY);
    mb_data.numbuttons = num_buttons = RARRAY_LEN(buttons);
    button_data = ALLOCA_N(SDL_MessageBoxButtonData, num_buttons);
    mb_data.buttons = button_data;
    texts = rb_ary_new2(num_buttons);
    
    for (i=0; i<num_buttons; ++i) {
        VALUE button = rb_ary_entry(buttons, i);
        VALUE flags = rb_hash_aref(button, sym_flags);
        VALUE text;
        if (flags == Qnil)
            button_data[i].flags = 0;
        else
            button_data[i].flags = NUM2INT(flags);
        text = rb_str_export_to_utf8(rb_hash_aref(button, sym_text));
        rb_ary_push(texts, text);
        button_data[i].buttonid = NUM2INT(rb_hash_aref(button, sym_id));
        button_data[i].text = StringValueCStr(text);
    }

    color_scheme = rb_hash_aref(params, sym_color_scheme);
    if (color_scheme == Qnil) {
        mb_data.colorScheme = NULL;
    } else {
        Check_Type(color_scheme, T_HASH);
        mb_data.colorScheme = &scheme;
        set_color_scheme(color_scheme, sym_bg, &scheme.colors[0]);
        set_color_scheme(color_scheme, sym_text, &scheme.colors[1]);
        set_color_scheme(color_scheme, sym_button_border, &scheme.colors[2]);
        set_color_scheme(color_scheme, sym_button_bg, &scheme.colors[3]);
        set_color_scheme(color_scheme, sym_button_selected, &scheme.colors[4]);
    }
    
    HANDLE_ERROR(SDL_ShowMessageBox(&mb_data, &buttonid));
    
    RB_GC_GUARD(title); RB_GC_GUARD(message); RB_GC_GUARD(texts);
    return INT2NUM(buttonid);
}

void rubysdl2_init_messagebox(void)
{
    rb_define_module_function(mSDL2, "show_simple_message_box",
                              SDL2_s_show_simple_message_box, 4);
    rb_define_module_function(mSDL2, "show_message_box", SDL2_s_show_message_box, 1);
    rb_define_const(mSDL2, "MESSAGEBOX_ERROR", INT2NUM(SDL_MESSAGEBOX_ERROR));
    rb_define_const(mSDL2, "MESSAGEBOX_WARNING", INT2NUM(SDL_MESSAGEBOX_WARNING));
    rb_define_const(mSDL2, "MESSAGEBOX_INFORMATION", INT2NUM(SDL_MESSAGEBOX_INFORMATION));
    rb_define_const(mSDL2, "MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT",
                    INT2NUM(SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT));
    rb_define_const(mSDL2, "MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT",
                    INT2NUM(SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT));

    sym_flags = ID2SYM(rb_intern("flags"));
    sym_window =  ID2SYM(rb_intern("window"));
    sym_message = ID2SYM(rb_intern("message"));
    sym_title = ID2SYM(rb_intern("title"));
    sym_buttons = ID2SYM(rb_intern("buttons"));
    sym_color_scheme = ID2SYM(rb_intern("color_scheme"));
    sym_id = ID2SYM(rb_intern("id"));
    sym_text = ID2SYM(rb_intern("text"));
    sym_bg = ID2SYM(rb_intern("bg"));
    sym_button_border = ID2SYM(rb_intern("button_border"));
    sym_button_bg = ID2SYM(rb_intern("button_bg"));
    sym_button_selected = ID2SYM(rb_intern("button_selected"));
}
