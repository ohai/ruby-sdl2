#include "rubysdl2_internal.h"
#include <SDL_messagebox.h>

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

void rubysdl2_init_messagebox(void)
{
    rb_define_module_function(mSDL2, "show_simple_message_box",
                              SDL2_s_show_simple_message_box, 4);

    rb_define_const(mSDL2, "MESSAGEBOX_ERROR", INT2NUM(SDL_MESSAGEBOX_ERROR));
    rb_define_const(mSDL2, "MESSAGEBOX_WARNING", INT2NUM(SDL_MESSAGEBOX_WARNING));
    rb_define_const(mSDL2, "MESSAGEBOX_INFORMATION", INT2NUM(SDL_MESSAGEBOX_INFORMATION));
}
