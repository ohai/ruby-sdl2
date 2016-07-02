#include "rubysdl2_internal.h"
#include <SDL_clipboard.h>

/*
 * Document-module: SDL2::Clipboard
 *
 * This module has clipboard manipulating functions.
 * 
 */

/*
 * Get the text from the clipboard.
 *
 * @return [String] a string in the clipboard
 * @return [nil] if the clipboard is empty
 */ 
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

/*
 * @overload text=(text) 
 *   Set the text in the clipboard.
 *   
 *   @param text [String] a new text
 *   @return [String] text
 */
static VALUE Clipboard_s_set_text(VALUE self, VALUE text)
{
    HANDLE_ERROR(SDL_SetClipboardText(StringValueCStr(text)));
    return text;
}

/*
 * Return true if the clipboard has any text.
 *
 */
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
