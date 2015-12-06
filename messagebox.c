#include "rubysdl2_internal.h"
#include <SDL_messagebox.h>

static VALUE sym_flags, sym_window, sym_title, sym_message, sym_buttons, sym_color_scheme,
    sym_id, sym_text, sym_bg, sym_button_border, sym_button_bg, sym_button_selected;

static VALUE mMessageBox;

static inline SDL_Window* Get_SDL_Window_or_NULL(VALUE win)
{
    if (win == Qnil)
        return NULL;
    else
        return Get_SDL_Window(win);
}

/*
 * Document-module: SDL2::MessageBox
 *
 * This module provides functions to show a modal message box.
 */

/*
 * @overload show_simple_box(flag, title, message, parent)
 *   Create a simple modal message box.
 *
 *   This function pauses all ruby's threads and
 *   the threads are resumed when modal dialog is closed.
 *   
 *   You can create a message box before calling {SDL2.init}.
 *   
 *   You specify one of the following constants as flag
 *
 *   * {SDL2::MessageBox::ERROR}
 *   * {SDL2::MessageBox::WARNING}
 *   * {SDL2::MessageBox::INFORMATION}
 *
 *   @example show warning dialog
 *   
 *       SDL2.show_simple_message_box(SDL2::MessageBox::WARNING, "warning!",
 *                                    "Somewhat special warning message!!", nil)
 *                                
 *   @param [Integer] flag one of the above flags
 *   @param [String] title title text
 *   @param [String] message message text
 *   @param [SDL2::Window,nil] parent the parent window, or nil for no parent
 *   @return [nil]
 *   
 *   @see .show
 */
static VALUE MessageBox_s_show_simple_box(VALUE self, VALUE flag, VALUE title,
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

/*
 * @overload show(flag:, window: nil, title:, message:, buttons:, color_scheme: nil)
 *   Create a model message box.
 *
 *   You specify one of the following constants as flag
 *
 *   * {SDL2::MessageBox::ERROR}
 *   * {SDL2::MessageBox::WARNING}
 *   * {SDL2::MessageBox::INFORMATION}
 *
 *   One button in the dialog represents a hash with folloing elements.
 *   
 *       { flags: 0, SDL2::MessageBox::BUTTON_ESCAPEKEY_DEFAULT, or
 *                SDL2::MessageBox::BUTTON_RETURNKEY_DEFAULT,
 *                you can ignore it for 0,
 *         text: text of a button,
 *         id: index of the button
 *       }
 *
 *   and buttons is an array of above button hashes.
 *
 *   You can specify the color of message box by color_scheme.
 *   color_scheme is an hash whose keys are :bg, :text, :button_border, :button_bg,
 *   and :button_selected and values are array of three integers representing
 *   color.
 *   You can also use default color scheme by giving nil.
 *   
 *   
 *   This function pauses all ruby's threads until
 *   the modal dialog is closed.
 *   
 *   You can create a message box before calling {SDL2.init}.
 *
 *   @example show a dialog with 3 buttons
 *       button = SDL2::MessageBox.show(flags: SDL2::MessageBox::WARNING,
 *                                      window: nil,
 *                                      title: "Warning window",
 *                                      message: "Here is the warning message",
 *                                      buttons: [ { # flags is ignored
 *                                                  id: 0, 
 *                                                  text: "No",
 *                                                 }, 
 *                                                 {flags: SDL2::MessageBox::BUTTON_RETURNKEY_DEFAULT,
 *                                                  id: 1,
 *                                                  text: "Yes",
 *                                                 },
 *                                                 {flags: SDL2::MessageBox::BUTTON_ESCAPEKEY_DEFAULT,
 *                                                  id: 2,
 *                                                  text: "Cancel",
 *                                                 },
 *                                               ],
 *                                      color_scheme: {
 *                                                     bg: [255, 0, 0],
 *                                                     text: [0, 255, 0],
 *                                                     button_border: [255, 0, 0],
 *                                                     button_bg: [0, 0, 255],
 *                                                     button_selected: [255, 0, 0]
 *                                                    }
 *                                     )
 *                                 
 *   @param [Integer] flags message box type flag
 *   @param [SDL2::Window,nil] window the parent window, or nil for no parent
 *   @param [String] title the title text
 *   @param [String] message the message text
 *   @param [Array<Hash<Symbol => Object>>] buttons array of buttons
 *   @param [Hash<Symbol=>[Integer,Integer,Integer]> nil] color_scheme
 *       color scheme, or nil for the default color scheme
 *   @return [Integer] pressed button id
 *   
 *   @see .show_simple_box
 */
static VALUE MessageBox_s_show(VALUE self, VALUE params)
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
    mMessageBox = rb_define_module_under(mSDL2, "MessageBox");
    
    rb_define_singleton_method(mMessageBox, "show_simple_box",
                               MessageBox_s_show_simple_box, 4);
    rb_define_singleton_method(mMessageBox, "show", MessageBox_s_show, 1);
    /* This flag means that the message box shows an error message */
    rb_define_const(mMessageBox, "ERROR", INT2NUM(SDL_MESSAGEBOX_ERROR));
    /* This flag means that the message box shows a warning message */
    rb_define_const(mMessageBox, "WARNING", INT2NUM(SDL_MESSAGEBOX_WARNING));
    /* This flag means that the message box shows an informational message */
    rb_define_const(mMessageBox, "INFORMATION", INT2NUM(SDL_MESSAGEBOX_INFORMATION));
    /* This flag represents the button is selected when return key is pressed */
    rb_define_const(mMessageBox, "BUTTON_RETURNKEY_DEFAULT",
                    INT2NUM(SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT));
    /* This flag represents the button is selected when escape key is pressed */
    rb_define_const(mMessageBox, "BUTTON_ESCAPEKEY_DEFAULT",
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
