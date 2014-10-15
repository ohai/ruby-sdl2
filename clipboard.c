#include "rubysdl2_internal.h"
#include <SDL_clipboard.h>

static VALUE Clipboard_s_text(VALUE self)
{
    if (SDL_HasClipboardText()) {
        char* text = SDL_GetClipboardText();
        VALUE str;
        if (!text)
            SDL_ERROR();
        str = utf8str_new_cstr(text);
        SDL_free(text);
        return str;
    } else {
        return Qnil;
    }
}

static VALUE Clipboard_s_set_text(VALUE self, VALUE text)
{
    HANDLE_ERROR(SDL_SetClipboardText(StringValueCStr(text)));
    return text;
}

static VALUE Clipboard_s_has_text_p(VALUE self)
{
    return INT2BOOL(SDL_HasClipboardText());
}

void rubysdl2_init_clipboard(void)
{
    VALUE mClipBoard = rb_define_module_under(mSDL2, "Clipboard");
    
    rb_define_module_function(mClipBoard, "text", Clipboard_s_text, 0);
    rb_define_module_function(mClipBoard, "text=", Clipboard_s_set_text, 1);
    rb_define_module_function(mClipBoard, "has_text?", Clipboard_s_has_text_p, 0);
}
