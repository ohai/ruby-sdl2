/* -*- C -*- */
#include "rubysdl2_internal.h"
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_scancode.h>

static VALUE mKey;
static VALUE mScan;
static VALUE mMod;
static VALUE mTextInput;

/*
 * Document-module: SDL2::Key
 *
 * This module has "virtual" keycode constants and some
 * keyboard input handling functions.
 *
 */

/*
 * @overload name_of(code)
 *   @param [Integer] code keycode
 *
 * Get a human-readable name for a key
 * 
 * @return [String] the name of given keycode
 * @see .keycode_from_name
 * @see SDL2::Key::Scan.name_of
 */
static VALUE Key_s_name_of(VALUE self, VALUE code)
{
    return utf8str_new_cstr(SDL_GetKeyName(NUM2INT(code)));
}

/*
 * @overload keycode_from_name(name)
 *   @param [String] name name of a key
 *
 * Get a key code from given name
 * @return [Integer] keycode
 * @see .name_of
 * @see SDL2::Key::Scan.from_name
 */
static VALUE Key_s_keycode_from_name(VALUE self, VALUE name)
{
    return INT2NUM(SDL_GetKeyFromName(StringValueCStr(name)));
}

/*
 * @overload keycode_from_scancode(scancode)
 *   @param [Integer] scancode scancode
 *
 * Convert a scancode to the corresponding keycode
 * 
 * @return [Integer]
 * @see SDL2::Key::Scan.from_keycode
 */
static VALUE Key_s_keycode_from_scancode(VALUE self, VALUE scancode)
{
    return INT2NUM(SDL_GetKeyFromScancode(NUM2INT(scancode)));
}

/*
 * @overload pressed?(code)
 *   @param [Integer] code scancode
 *
 * Get whether the key of the given scancode is pressed or not.
 */
static VALUE Key_s_pressed_p(VALUE self, VALUE code)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Scancode scancode;
    if (!state) {
        SDL_PumpEvents();
        state = SDL_GetKeyboardState(NULL);
        if (!state)
            rb_raise(eSDL2Error, "Event subsystem is not initialized");
    }
    scancode = NUM2UINT(code);
    if (scancode >= SDL_NUM_SCANCODES) 
        rb_raise(rb_eArgError, "too large scancode %d", scancode);
    
    return INT2BOOL(state[scancode]);
}

/*
 * Document-module: SDL2::Key::Scan
 *
 * This module has "physical" key scancode constants and some
 * scancode handling functions.
 */

/*
 * @overload name_of(code)
 *   @param [Integer] code scancode
 *
 * Get a human-readable name of the given scancode
 * @return [String]
 * @see SDL2::Key.name_of
 * @see .from_name
 */
static VALUE Scan_s_name_of(VALUE self, VALUE code)
{
    return utf8str_new_cstr(SDL_GetScancodeName(NUM2INT(code)));
}

/*
 * @overload from_name(name)
 *   @param [String] name name of a key
 *
 * Get a scancode from the key of the given name
 * @return [String]
 * @see .name_of
 * @see SDL2::Key.keycode_from_name
 */
static VALUE Scan_s_from_name(VALUE self, VALUE name)
{
    return INT2NUM(SDL_GetScancodeFromName(StringValueCStr(name)));
}

/*
 * @overload from_keycode(keycode)
 *   @param [Integer] keycode keycode
 *
 * Get a keycode corresponding to the given keycode
 * @return [Integer] keycode
 * @see SDL2::Key.keycode_from_scancode
 */
static VALUE Scan_s_from_keycode(VALUE self, VALUE keycode)
{
    return INT2NUM(SDL_GetScancodeFromKey(NUM2INT(keycode)));
}

/*
 * Document-module: SDL2::Key::Mod
 *
 * This module has key modifier bitmask constants and
 * some functions to handle key modifier states.
 */

/*
 * Get the current key modifier state
 * 
 * You can examine whether the modifier key is pressed
 * using bitmask constants of {SDL2::Key::Mod}.
 * 
 * @return [Integer] key state
 */
static VALUE Mod_s_state(VALUE self)
{
    return UINT2NUM(SDL_GetModState());
}

/*
 * @overload state=(keymod)
 *   @param [Integer] keymod key modifier flags (bits)
 *   
 * Set the current key modifier state
 * 
 * @note This does not change the keyboard state, only the key modifier flags.
 * @return [keymod]
 * @see .state
 */
static VALUE Mod_s_set_state(VALUE self, VALUE keymod)
{
    SDL_SetModState(NUM2UINT(keymod));
    return Qnil;
}

/*
 * Document-module: SDL2::TextInput
 *
 * This module provides Unicode text input support.
 *
 * Normally, you can handle key inputs from key events
 * and {SDL2::Key} module. This module is required to
 * input thousands kinds of symbols like CJK languages.
 * Please see {https://wiki.libsdl.org/Tutorials/TextInput}
 * to understand the concept of Unicode text input.
 */

/*
 * Return true if Unicode text input events are enabled.
 *
 * @see .start
 * @see .stop
 */
static VALUE TextInput_s_active_p(VALUE self)
{
    return INT2BOOL(SDL_IsTextInputActive());
}

/*
 * Enable Unicode input events.
 *
 * @return [nil]
 * @see .stop
 * @see .active?
 */
static VALUE TextInput_s_start(VALUE self)
{
    SDL_StartTextInput(); return Qnil;
}

/*
 * Disable Unicode input events.
 *
 * @return [nil]
 * @see .start
 * @see .active?
 */
static VALUE TextInput_s_stop(VALUE self)
{
    SDL_StopTextInput(); return Qnil;
}

/*
 * @overload rect=(rect)
 *   Set the rectanlgle used to type Unicode text inputs.
 *
 *   @param rect [SDL2::Rect] the rectangle to receive text
 *   @return [rect]
 */
static VALUE TextInput_s_set_rect(VALUE self, VALUE rect)
{
    SDL_Rect *r = Get_SDL_Rect(rect);
    SDL_SetTextInputRect(r);
    return rect;
}

/*

    





    

    



*/
void rubysdl2_init_key(void)
{
    mKey = rb_define_module_under(mSDL2, "Key");
    mScan = rb_define_module_under(mKey, "Scan");
    mMod = rb_define_module_under(mKey, "Mod");
    mTextInput = rb_define_module_under(mSDL2, "TextInput");
    
    rb_define_module_function(mKey, "name_of", Key_s_name_of, 1);
    rb_define_module_function(mKey, "keycode_from_name", Key_s_keycode_from_name, 1);
    rb_define_module_function(mKey, "keycode_from_scancode", Key_s_keycode_from_scancode, 1);
    rb_define_module_function(mKey, "pressed?", Key_s_pressed_p, 1);
    rb_define_module_function(mScan, "name_of", Scan_s_name_of, 1);
    rb_define_module_function(mScan, "from_name", Scan_s_from_name, 1);
    rb_define_module_function(mScan, "from_keycode", Scan_s_from_keycode, 1);
    rb_define_module_function(mMod, "state", Mod_s_state, 0);
    rb_define_module_function(mMod, "state=", Mod_s_set_state, 1);
    rb_define_module_function(mTextInput, "active?", TextInput_s_active_p, 0);
    rb_define_module_function(mTextInput, "start", TextInput_s_start, 0);
    rb_define_module_function(mTextInput, "stop", TextInput_s_stop, 0);
    rb_define_module_function(mTextInput, "rect=", TextInput_s_set_rect, 1);
    
    /* @return [Integer] unused scancode */
    rb_define_const(mScan, "UNKNOWN", INT2NUM(SDL_SCANCODE_UNKNOWN));
    /* @return [Integer] scancode for alphabet key "A" */
    rb_define_const(mScan, "A", INT2NUM(SDL_SCANCODE_A));
    /* @return [Integer] scancode for alphabet key "B" */
    rb_define_const(mScan, "B", INT2NUM(SDL_SCANCODE_B));
    /* @return [Integer] scancode for alphabet key "C" */
    rb_define_const(mScan, "C", INT2NUM(SDL_SCANCODE_C));
    /* @return [Integer] scancode for alphabet key "D" */
    rb_define_const(mScan, "D", INT2NUM(SDL_SCANCODE_D));
    /* @return [Integer] scancode for alphabet key "E" */
    rb_define_const(mScan, "E", INT2NUM(SDL_SCANCODE_E));
    /* @return [Integer] scancode for alphabet key "F" */
    rb_define_const(mScan, "F", INT2NUM(SDL_SCANCODE_F));
    /* @return [Integer] scancode for alphabet key "G" */
    rb_define_const(mScan, "G", INT2NUM(SDL_SCANCODE_G));
    /* @return [Integer] scancode for alphabet key "H" */
    rb_define_const(mScan, "H", INT2NUM(SDL_SCANCODE_H));
    /* @return [Integer] scancode for alphabet key "I" */
    rb_define_const(mScan, "I", INT2NUM(SDL_SCANCODE_I));
    /* @return [Integer] scancode for alphabet key "J" */
    rb_define_const(mScan, "J", INT2NUM(SDL_SCANCODE_J));
    /* @return [Integer] scancode for alphabet key "K" */
    rb_define_const(mScan, "K", INT2NUM(SDL_SCANCODE_K));
    /* @return [Integer] scancode for alphabet key "L" */
    rb_define_const(mScan, "L", INT2NUM(SDL_SCANCODE_L));
    /* @return [Integer] scancode for alphabet key "M" */
    rb_define_const(mScan, "M", INT2NUM(SDL_SCANCODE_M));
    /* @return [Integer] scancode for alphabet key "N" */
    rb_define_const(mScan, "N", INT2NUM(SDL_SCANCODE_N));
    /* @return [Integer] scancode for alphabet key "O" */
    rb_define_const(mScan, "O", INT2NUM(SDL_SCANCODE_O));
    /* @return [Integer] scancode for alphabet key "P" */
    rb_define_const(mScan, "P", INT2NUM(SDL_SCANCODE_P));
    /* @return [Integer] scancode for alphabet key "Q" */
    rb_define_const(mScan, "Q", INT2NUM(SDL_SCANCODE_Q));
    /* @return [Integer] scancode for alphabet key "R" */
    rb_define_const(mScan, "R", INT2NUM(SDL_SCANCODE_R));
    /* @return [Integer] scancode for alphabet key "S" */
    rb_define_const(mScan, "S", INT2NUM(SDL_SCANCODE_S));
    /* @return [Integer] scancode for alphabet key "T" */
    rb_define_const(mScan, "T", INT2NUM(SDL_SCANCODE_T));
    /* @return [Integer] scancode for alphabet key "U" */
    rb_define_const(mScan, "U", INT2NUM(SDL_SCANCODE_U));
    /* @return [Integer] scancode for alphabet key "V" */
    rb_define_const(mScan, "V", INT2NUM(SDL_SCANCODE_V));
    /* @return [Integer] scancode for alphabet key "W" */
    rb_define_const(mScan, "W", INT2NUM(SDL_SCANCODE_W));
    /* @return [Integer] scancode for alphabet key "X" */
    rb_define_const(mScan, "X", INT2NUM(SDL_SCANCODE_X));
    /* @return [Integer] scancode for alphabet key "Y" */
    rb_define_const(mScan, "Y", INT2NUM(SDL_SCANCODE_Y));
    /* @return [Integer] scancode for alphabet key "Z" */
    rb_define_const(mScan, "Z", INT2NUM(SDL_SCANCODE_Z));
    
    /* @return [Integer] scancode for number key "1" (not on keypad) */
    rb_define_const(mScan, "K1", INT2NUM(SDL_SCANCODE_1));
    /* @return [Integer] scancode for number key "2" (not on keypad) */
    rb_define_const(mScan, "K2", INT2NUM(SDL_SCANCODE_2));
    /* @return [Integer] scancode for number key "3" (not on keypad) */
    rb_define_const(mScan, "K3", INT2NUM(SDL_SCANCODE_3));
    /* @return [Integer] scancode for number key "4" (not on keypad) */
    rb_define_const(mScan, "K4", INT2NUM(SDL_SCANCODE_4));
    /* @return [Integer] scancode for number key "5" (not on keypad) */
    rb_define_const(mScan, "K5", INT2NUM(SDL_SCANCODE_5));
    /* @return [Integer] scancode for number key "6" (not on keypad) */
    rb_define_const(mScan, "K6", INT2NUM(SDL_SCANCODE_6));
    /* @return [Integer] scancode for number key "7" (not on keypad) */
    rb_define_const(mScan, "K7", INT2NUM(SDL_SCANCODE_7));
    /* @return [Integer] scancode for number key "8" (not on keypad) */
    rb_define_const(mScan, "K8", INT2NUM(SDL_SCANCODE_8));
    /* @return [Integer] scancode for number key "9" (not on keypad) */
    rb_define_const(mScan, "K9", INT2NUM(SDL_SCANCODE_9));
    /* @return [Integer] scancode for number key "0" (not on keypad) */
    rb_define_const(mScan, "K0", INT2NUM(SDL_SCANCODE_0));

    /* @return [Integer] scancode for "RETURN" key */
    rb_define_const(mScan, "RETURN", INT2NUM(SDL_SCANCODE_RETURN));
    /* @return [Integer] scancode for "ESCAPE" key */
    rb_define_const(mScan, "ESCAPE", INT2NUM(SDL_SCANCODE_ESCAPE));
    /* @return [Integer] scancode for "BACKSPACE" key */
    rb_define_const(mScan, "BACKSPACE", INT2NUM(SDL_SCANCODE_BACKSPACE));
    /* @return [Integer] scancode for "TAB" key */
    rb_define_const(mScan, "TAB", INT2NUM(SDL_SCANCODE_TAB));
    /* @return [Integer] scancode for "SPACE" key */
    rb_define_const(mScan, "SPACE", INT2NUM(SDL_SCANCODE_SPACE));

    /* @return [Integer] scancode for "MINUS" key */
    rb_define_const(mScan, "MINUS", INT2NUM(SDL_SCANCODE_MINUS));
    /* @return [Integer] scancode for "EQUALS" key */
    rb_define_const(mScan, "EQUALS", INT2NUM(SDL_SCANCODE_EQUALS));
    /* @return [Integer] scancode for "LEFTBRACKET" key */
    rb_define_const(mScan, "LEFTBRACKET", INT2NUM(SDL_SCANCODE_LEFTBRACKET));
    /* @return [Integer] scancode for "RIGHTBRACKET" key */
    rb_define_const(mScan, "RIGHTBRACKET", INT2NUM(SDL_SCANCODE_RIGHTBRACKET));
    /* @return [Integer] scancode for "BACKSLASH" key */
    rb_define_const(mScan, "BACKSLASH", INT2NUM(SDL_SCANCODE_BACKSLASH));

    /* @return [Integer] scancode for "NONUSHASH" key */
    rb_define_const(mScan, "NONUSHASH", INT2NUM(SDL_SCANCODE_NONUSHASH));
    /* @return [Integer] scancode for "SEMICOLON" key */
    rb_define_const(mScan, "SEMICOLON", INT2NUM(SDL_SCANCODE_SEMICOLON));
    /* @return [Integer] scancode for "APOSTROPHE" key */
    rb_define_const(mScan, "APOSTROPHE", INT2NUM(SDL_SCANCODE_APOSTROPHE));
    /* @return [Integer] scancode for "GRAVE" key */
    rb_define_const(mScan, "GRAVE", INT2NUM(SDL_SCANCODE_GRAVE));
    /* @return [Integer] scancode for "COMMA" key */
    rb_define_const(mScan, "COMMA", INT2NUM(SDL_SCANCODE_COMMA));
    /* @return [Integer] scancode for "PERIOD" key */
    rb_define_const(mScan, "PERIOD", INT2NUM(SDL_SCANCODE_PERIOD));
    /* @return [Integer] scancode for "SLASH" key */
    rb_define_const(mScan, "SLASH", INT2NUM(SDL_SCANCODE_SLASH));

    /* @return [Integer] scancode for "CAPSLOCK" key */
    rb_define_const(mScan, "CAPSLOCK", INT2NUM(SDL_SCANCODE_CAPSLOCK));

    /* @return [Integer] scancode for "F1" key */
    rb_define_const(mScan, "F1", INT2NUM(SDL_SCANCODE_F1));
    /* @return [Integer] scancode for "F2" key */
    rb_define_const(mScan, "F2", INT2NUM(SDL_SCANCODE_F2));
    /* @return [Integer] scancode for "F3" key */
    rb_define_const(mScan, "F3", INT2NUM(SDL_SCANCODE_F3));
    /* @return [Integer] scancode for "F4" key */
    rb_define_const(mScan, "F4", INT2NUM(SDL_SCANCODE_F4));
    /* @return [Integer] scancode for "F5" key */
    rb_define_const(mScan, "F5", INT2NUM(SDL_SCANCODE_F5));
    /* @return [Integer] scancode for "F6" key */
    rb_define_const(mScan, "F6", INT2NUM(SDL_SCANCODE_F6));
    /* @return [Integer] scancode for "F7" key */
    rb_define_const(mScan, "F7", INT2NUM(SDL_SCANCODE_F7));
    /* @return [Integer] scancode for "F8" key */
    rb_define_const(mScan, "F8", INT2NUM(SDL_SCANCODE_F8));
    /* @return [Integer] scancode for "F9" key */
    rb_define_const(mScan, "F9", INT2NUM(SDL_SCANCODE_F9));
    /* @return [Integer] scancode for "F10" key */
    rb_define_const(mScan, "F10", INT2NUM(SDL_SCANCODE_F10));
    /* @return [Integer] scancode for "F11" key */
    rb_define_const(mScan, "F11", INT2NUM(SDL_SCANCODE_F11));
    /* @return [Integer] scancode for "F12" key */
    rb_define_const(mScan, "F12", INT2NUM(SDL_SCANCODE_F12));

    /* @return [Integer] scancode for "PRINTSCREEN" key */
    rb_define_const(mScan, "PRINTSCREEN", INT2NUM(SDL_SCANCODE_PRINTSCREEN));
    /* @return [Integer] scancode for "SCROLLLOCK" key */
    rb_define_const(mScan, "SCROLLLOCK", INT2NUM(SDL_SCANCODE_SCROLLLOCK));
    /* @return [Integer] scancode for "PAUSE" key */
    rb_define_const(mScan, "PAUSE", INT2NUM(SDL_SCANCODE_PAUSE));
    /* @return [Integer] scancode for "INSERT" key */
    rb_define_const(mScan, "INSERT", INT2NUM(SDL_SCANCODE_INSERT));

    /* @return [Integer] scancode for "HOME" key */
    rb_define_const(mScan, "HOME", INT2NUM(SDL_SCANCODE_HOME));
    /* @return [Integer] scancode for "PAGEUP" key */
    rb_define_const(mScan, "PAGEUP", INT2NUM(SDL_SCANCODE_PAGEUP));
    /* @return [Integer] scancode for "DELETE" key */
    rb_define_const(mScan, "DELETE", INT2NUM(SDL_SCANCODE_DELETE));
    /* @return [Integer] scancode for "END" key */
    rb_define_const(mScan, "END", INT2NUM(SDL_SCANCODE_END));
    /* @return [Integer] scancode for "PAGEDOWN" key */
    rb_define_const(mScan, "PAGEDOWN", INT2NUM(SDL_SCANCODE_PAGEDOWN));
    /* @return [Integer] scancode for "RIGHT" key */
    rb_define_const(mScan, "RIGHT", INT2NUM(SDL_SCANCODE_RIGHT));
    /* @return [Integer] scancode for "LEFT" key */
    rb_define_const(mScan, "LEFT", INT2NUM(SDL_SCANCODE_LEFT));
    /* @return [Integer] scancode for "DOWN" key */
    rb_define_const(mScan, "DOWN", INT2NUM(SDL_SCANCODE_DOWN));
    /* @return [Integer] scancode for "UP" key */
    rb_define_const(mScan, "UP", INT2NUM(SDL_SCANCODE_UP));

    /* @return [Integer] scancode for "NUMLOCKCLEAR" key */
    rb_define_const(mScan, "NUMLOCKCLEAR", INT2NUM(SDL_SCANCODE_NUMLOCKCLEAR));

    /* @return [Integer] scancode for "KP_DIVIDE" key */
    rb_define_const(mScan, "KP_DIVIDE", INT2NUM(SDL_SCANCODE_KP_DIVIDE));
    /* @return [Integer] scancode for "KP_MULTIPLY" key */
    rb_define_const(mScan, "KP_MULTIPLY", INT2NUM(SDL_SCANCODE_KP_MULTIPLY));
    /* @return [Integer] scancode for "KP_MINUS" key */
    rb_define_const(mScan, "KP_MINUS", INT2NUM(SDL_SCANCODE_KP_MINUS));
    /* @return [Integer] scancode for "KP_PLUS" key */
    rb_define_const(mScan, "KP_PLUS", INT2NUM(SDL_SCANCODE_KP_PLUS));
    /* @return [Integer] scancode for "KP_ENTER" key */
    rb_define_const(mScan, "KP_ENTER", INT2NUM(SDL_SCANCODE_KP_ENTER));
    /* @return [Integer] scancode for "KP_1" key */
    rb_define_const(mScan, "KP_1", INT2NUM(SDL_SCANCODE_KP_1));
    /* @return [Integer] scancode for "KP_2" key */
    rb_define_const(mScan, "KP_2", INT2NUM(SDL_SCANCODE_KP_2));
    /* @return [Integer] scancode for "KP_3" key */
    rb_define_const(mScan, "KP_3", INT2NUM(SDL_SCANCODE_KP_3));
    /* @return [Integer] scancode for "KP_4" key */
    rb_define_const(mScan, "KP_4", INT2NUM(SDL_SCANCODE_KP_4));
    /* @return [Integer] scancode for "KP_5" key */
    rb_define_const(mScan, "KP_5", INT2NUM(SDL_SCANCODE_KP_5));
    /* @return [Integer] scancode for "KP_6" key */
    rb_define_const(mScan, "KP_6", INT2NUM(SDL_SCANCODE_KP_6));
    /* @return [Integer] scancode for "KP_7" key */
    rb_define_const(mScan, "KP_7", INT2NUM(SDL_SCANCODE_KP_7));
    /* @return [Integer] scancode for "KP_8" key */
    rb_define_const(mScan, "KP_8", INT2NUM(SDL_SCANCODE_KP_8));
    /* @return [Integer] scancode for "KP_9" key */
    rb_define_const(mScan, "KP_9", INT2NUM(SDL_SCANCODE_KP_9));
    /* @return [Integer] scancode for "KP_0" key */
    rb_define_const(mScan, "KP_0", INT2NUM(SDL_SCANCODE_KP_0));
    /* @return [Integer] scancode for "KP_PERIOD" key */
    rb_define_const(mScan, "KP_PERIOD", INT2NUM(SDL_SCANCODE_KP_PERIOD));

    /* @return [Integer] scancode for "NONUSBACKSLASH" key */
    rb_define_const(mScan, "NONUSBACKSLASH", INT2NUM(SDL_SCANCODE_NONUSBACKSLASH));
    /* @return [Integer] scancode for "APPLICATION" key */
    rb_define_const(mScan, "APPLICATION", INT2NUM(SDL_SCANCODE_APPLICATION));
    /* @return [Integer] scancode for "POWER" key */
    rb_define_const(mScan, "POWER", INT2NUM(SDL_SCANCODE_POWER));
    /* @return [Integer] scancode for "KP_EQUALS" key */
    rb_define_const(mScan, "KP_EQUALS", INT2NUM(SDL_SCANCODE_KP_EQUALS));
    /* @return [Integer] scancode for "F13" key */
    rb_define_const(mScan, "F13", INT2NUM(SDL_SCANCODE_F13));
    /* @return [Integer] scancode for "F14" key */
    rb_define_const(mScan, "F14", INT2NUM(SDL_SCANCODE_F14));
    /* @return [Integer] scancode for "F15" key */
    rb_define_const(mScan, "F15", INT2NUM(SDL_SCANCODE_F15));
    /* @return [Integer] scancode for "F16" key */
    rb_define_const(mScan, "F16", INT2NUM(SDL_SCANCODE_F16));
    /* @return [Integer] scancode for "F17" key */
    rb_define_const(mScan, "F17", INT2NUM(SDL_SCANCODE_F17));
    /* @return [Integer] scancode for "F18" key */
    rb_define_const(mScan, "F18", INT2NUM(SDL_SCANCODE_F18));
    /* @return [Integer] scancode for "F19" key */
    rb_define_const(mScan, "F19", INT2NUM(SDL_SCANCODE_F19));
    /* @return [Integer] scancode for "F20" key */
    rb_define_const(mScan, "F20", INT2NUM(SDL_SCANCODE_F20));
    /* @return [Integer] scancode for "F21" key */
    rb_define_const(mScan, "F21", INT2NUM(SDL_SCANCODE_F21));
    /* @return [Integer] scancode for "F22" key */
    rb_define_const(mScan, "F22", INT2NUM(SDL_SCANCODE_F22));
    /* @return [Integer] scancode for "F23" key */
    rb_define_const(mScan, "F23", INT2NUM(SDL_SCANCODE_F23));
    /* @return [Integer] scancode for "F24" key */
    rb_define_const(mScan, "F24", INT2NUM(SDL_SCANCODE_F24));
    /* @return [Integer] scancode for "EXECUTE" key */
    rb_define_const(mScan, "EXECUTE", INT2NUM(SDL_SCANCODE_EXECUTE));
    /* @return [Integer] scancode for "HELP" key */
    rb_define_const(mScan, "HELP", INT2NUM(SDL_SCANCODE_HELP));
    /* @return [Integer] scancode for "MENU" key */
    rb_define_const(mScan, "MENU", INT2NUM(SDL_SCANCODE_MENU));
    /* @return [Integer] scancode for "SELECT" key */
    rb_define_const(mScan, "SELECT", INT2NUM(SDL_SCANCODE_SELECT));
    /* @return [Integer] scancode for "STOP" key */
    rb_define_const(mScan, "STOP", INT2NUM(SDL_SCANCODE_STOP));
    /* @return [Integer] scancode for "AGAIN" key */
    rb_define_const(mScan, "AGAIN", INT2NUM(SDL_SCANCODE_AGAIN));
    /* @return [Integer] scancode for "UNDO" key */
    rb_define_const(mScan, "UNDO", INT2NUM(SDL_SCANCODE_UNDO));
    /* @return [Integer] scancode for "CUT" key */
    rb_define_const(mScan, "CUT", INT2NUM(SDL_SCANCODE_CUT));
    /* @return [Integer] scancode for "COPY" key */
    rb_define_const(mScan, "COPY", INT2NUM(SDL_SCANCODE_COPY));
    /* @return [Integer] scancode for "PASTE" key */
    rb_define_const(mScan, "PASTE", INT2NUM(SDL_SCANCODE_PASTE));
    /* @return [Integer] scancode for "FIND" key */
    rb_define_const(mScan, "FIND", INT2NUM(SDL_SCANCODE_FIND));
    /* @return [Integer] scancode for "MUTE" key */
    rb_define_const(mScan, "MUTE", INT2NUM(SDL_SCANCODE_MUTE));
    /* @return [Integer] scancode for "VOLUMEUP" key */
    rb_define_const(mScan, "VOLUMEUP", INT2NUM(SDL_SCANCODE_VOLUMEUP));
    /* @return [Integer] scancode for "VOLUMEDOWN" key */
    rb_define_const(mScan, "VOLUMEDOWN", INT2NUM(SDL_SCANCODE_VOLUMEDOWN));
    /* not sure whether there's a reason to enable these */
    /*     SDL_SCANCODE_LOCKINGCAPSLOCK = 130,  */
    /*     SDL_SCANCODE_LOCKINGNUMLOCK = 131, */
    /*     SDL_SCANCODE_LOCKINGSCROLLLOCK = 132, */
    /* @return [Integer] scancode for "KP_COMMA" key */
    rb_define_const(mScan, "KP_COMMA", INT2NUM(SDL_SCANCODE_KP_COMMA));
    /* @return [Integer] scancode for "KP_EQUALSAS400" key */
    rb_define_const(mScan, "KP_EQUALSAS400", INT2NUM(SDL_SCANCODE_KP_EQUALSAS400));

    /* @return [Integer] scancode for "INTERNATIONAL1" key */
    rb_define_const(mScan, "INTERNATIONAL1", INT2NUM(SDL_SCANCODE_INTERNATIONAL1));
        
    /* @return [Integer] scancode for "INTERNATIONAL2" key */
    rb_define_const(mScan, "INTERNATIONAL2", INT2NUM(SDL_SCANCODE_INTERNATIONAL2));
    /* @return [Integer] scancode for "INTERNATIONAL3" key */
    rb_define_const(mScan, "INTERNATIONAL3", INT2NUM(SDL_SCANCODE_INTERNATIONAL3));
    /* @return [Integer] scancode for "INTERNATIONAL4" key */
    rb_define_const(mScan, "INTERNATIONAL4", INT2NUM(SDL_SCANCODE_INTERNATIONAL4));
    /* @return [Integer] scancode for "INTERNATIONAL5" key */
    rb_define_const(mScan, "INTERNATIONAL5", INT2NUM(SDL_SCANCODE_INTERNATIONAL5));
    /* @return [Integer] scancode for "INTERNATIONAL6" key */
    rb_define_const(mScan, "INTERNATIONAL6", INT2NUM(SDL_SCANCODE_INTERNATIONAL6));
    /* @return [Integer] scancode for "INTERNATIONAL7" key */
    rb_define_const(mScan, "INTERNATIONAL7", INT2NUM(SDL_SCANCODE_INTERNATIONAL7));
    /* @return [Integer] scancode for "INTERNATIONAL8" key */
    rb_define_const(mScan, "INTERNATIONAL8", INT2NUM(SDL_SCANCODE_INTERNATIONAL8));
    /* @return [Integer] scancode for "INTERNATIONAL9" key */
    rb_define_const(mScan, "INTERNATIONAL9", INT2NUM(SDL_SCANCODE_INTERNATIONAL9));
    /* @return [Integer] scancode for "LANG1" key */
    rb_define_const(mScan, "LANG1", INT2NUM(SDL_SCANCODE_LANG1));
    /* @return [Integer] scancode for "LANG2" key */
    rb_define_const(mScan, "LANG2", INT2NUM(SDL_SCANCODE_LANG2));
    /* @return [Integer] scancode for "LANG3" key */
    rb_define_const(mScan, "LANG3", INT2NUM(SDL_SCANCODE_LANG3));
    /* @return [Integer] scancode for "LANG4" key */
    rb_define_const(mScan, "LANG4", INT2NUM(SDL_SCANCODE_LANG4));
    /* @return [Integer] scancode for "LANG5" key */
    rb_define_const(mScan, "LANG5", INT2NUM(SDL_SCANCODE_LANG5));
    /* @return [Integer] scancode for "LANG6" key */
    rb_define_const(mScan, "LANG6", INT2NUM(SDL_SCANCODE_LANG6));
    /* @return [Integer] scancode for "LANG7" key */
    rb_define_const(mScan, "LANG7", INT2NUM(SDL_SCANCODE_LANG7));
    /* @return [Integer] scancode for "LANG8" key */
    rb_define_const(mScan, "LANG8", INT2NUM(SDL_SCANCODE_LANG8));
    /* @return [Integer] scancode for "LANG9" key */
    rb_define_const(mScan, "LANG9", INT2NUM(SDL_SCANCODE_LANG9));

    /* @return [Integer] scancode for "ALTERASE" key */
    rb_define_const(mScan, "ALTERASE", INT2NUM(SDL_SCANCODE_ALTERASE));
    /* @return [Integer] scancode for "SYSREQ" key */
    rb_define_const(mScan, "SYSREQ", INT2NUM(SDL_SCANCODE_SYSREQ));
    /* @return [Integer] scancode for "CANCEL" key */
    rb_define_const(mScan, "CANCEL", INT2NUM(SDL_SCANCODE_CANCEL));
    /* @return [Integer] scancode for "CLEAR" key */
    rb_define_const(mScan, "CLEAR", INT2NUM(SDL_SCANCODE_CLEAR));
    /* @return [Integer] scancode for "PRIOR" key */
    rb_define_const(mScan, "PRIOR", INT2NUM(SDL_SCANCODE_PRIOR));
    /* @return [Integer] scancode for "RETURN2" key */
    rb_define_const(mScan, "RETURN2", INT2NUM(SDL_SCANCODE_RETURN2));
    /* @return [Integer] scancode for "SEPARATOR" key */
    rb_define_const(mScan, "SEPARATOR", INT2NUM(SDL_SCANCODE_SEPARATOR));
    /* @return [Integer] scancode for "OUT" key */
    rb_define_const(mScan, "OUT", INT2NUM(SDL_SCANCODE_OUT));
    /* @return [Integer] scancode for "OPER" key */
    rb_define_const(mScan, "OPER", INT2NUM(SDL_SCANCODE_OPER));
    /* @return [Integer] scancode for "CLEARAGAIN" key */
    rb_define_const(mScan, "CLEARAGAIN", INT2NUM(SDL_SCANCODE_CLEARAGAIN));
    /* @return [Integer] scancode for "CRSEL" key */
    rb_define_const(mScan, "CRSEL", INT2NUM(SDL_SCANCODE_CRSEL));
    /* @return [Integer] scancode for "EXSEL" key */
    rb_define_const(mScan, "EXSEL", INT2NUM(SDL_SCANCODE_EXSEL));

    /* @return [Integer] scancode for "KP_00" key */
    rb_define_const(mScan, "KP_00", INT2NUM(SDL_SCANCODE_KP_00));
    /* @return [Integer] scancode for "KP_000" key */
    rb_define_const(mScan, "KP_000", INT2NUM(SDL_SCANCODE_KP_000));
    /* @return [Integer] scancode for "THOUSANDSSEPARATOR" key */
    rb_define_const(mScan, "THOUSANDSSEPARATOR", INT2NUM(SDL_SCANCODE_THOUSANDSSEPARATOR));
    /* @return [Integer] scancode for "DECIMALSEPARATOR" key */
    rb_define_const(mScan, "DECIMALSEPARATOR", INT2NUM(SDL_SCANCODE_DECIMALSEPARATOR));
    /* @return [Integer] scancode for "CURRENCYUNIT" key */
    rb_define_const(mScan, "CURRENCYUNIT", INT2NUM(SDL_SCANCODE_CURRENCYUNIT));
    /* @return [Integer] scancode for "CURRENCYSUBUNIT" key */
    rb_define_const(mScan, "CURRENCYSUBUNIT", INT2NUM(SDL_SCANCODE_CURRENCYSUBUNIT));
    /* @return [Integer] scancode for "KP_LEFTPAREN" key */
    rb_define_const(mScan, "KP_LEFTPAREN", INT2NUM(SDL_SCANCODE_KP_LEFTPAREN));
    /* @return [Integer] scancode for "KP_RIGHTPAREN" key */
    rb_define_const(mScan, "KP_RIGHTPAREN", INT2NUM(SDL_SCANCODE_KP_RIGHTPAREN));
    /* @return [Integer] scancode for "KP_LEFTBRACE" key */
    rb_define_const(mScan, "KP_LEFTBRACE", INT2NUM(SDL_SCANCODE_KP_LEFTBRACE));
    /* @return [Integer] scancode for "KP_RIGHTBRACE" key */
    rb_define_const(mScan, "KP_RIGHTBRACE", INT2NUM(SDL_SCANCODE_KP_RIGHTBRACE));
    /* @return [Integer] scancode for "KP_TAB" key */
    rb_define_const(mScan, "KP_TAB", INT2NUM(SDL_SCANCODE_KP_TAB));
    /* @return [Integer] scancode for "KP_BACKSPACE" key */
    rb_define_const(mScan, "KP_BACKSPACE", INT2NUM(SDL_SCANCODE_KP_BACKSPACE));
    /* @return [Integer] scancode for "KP_A" key */
    rb_define_const(mScan, "KP_A", INT2NUM(SDL_SCANCODE_KP_A));
    /* @return [Integer] scancode for "KP_B" key */
    rb_define_const(mScan, "KP_B", INT2NUM(SDL_SCANCODE_KP_B));
    /* @return [Integer] scancode for "KP_C" key */
    rb_define_const(mScan, "KP_C", INT2NUM(SDL_SCANCODE_KP_C));
    /* @return [Integer] scancode for "KP_D" key */
    rb_define_const(mScan, "KP_D", INT2NUM(SDL_SCANCODE_KP_D));
    /* @return [Integer] scancode for "KP_E" key */
    rb_define_const(mScan, "KP_E", INT2NUM(SDL_SCANCODE_KP_E));
    /* @return [Integer] scancode for "KP_F" key */
    rb_define_const(mScan, "KP_F", INT2NUM(SDL_SCANCODE_KP_F));
    /* @return [Integer] scancode for "KP_XOR" key */
    rb_define_const(mScan, "KP_XOR", INT2NUM(SDL_SCANCODE_KP_XOR));
    /* @return [Integer] scancode for "KP_POWER" key */
    rb_define_const(mScan, "KP_POWER", INT2NUM(SDL_SCANCODE_KP_POWER));
    /* @return [Integer] scancode for "KP_PERCENT" key */
    rb_define_const(mScan, "KP_PERCENT", INT2NUM(SDL_SCANCODE_KP_PERCENT));
    /* @return [Integer] scancode for "KP_LESS" key */
    rb_define_const(mScan, "KP_LESS", INT2NUM(SDL_SCANCODE_KP_LESS));
    /* @return [Integer] scancode for "KP_GREATER" key */
    rb_define_const(mScan, "KP_GREATER", INT2NUM(SDL_SCANCODE_KP_GREATER));
    /* @return [Integer] scancode for "KP_AMPERSAND" key */
    rb_define_const(mScan, "KP_AMPERSAND", INT2NUM(SDL_SCANCODE_KP_AMPERSAND));
    /* @return [Integer] scancode for "KP_DBLAMPERSAND" key */
    rb_define_const(mScan, "KP_DBLAMPERSAND", INT2NUM(SDL_SCANCODE_KP_DBLAMPERSAND));
    /* @return [Integer] scancode for "KP_VERTICALBAR" key */
    rb_define_const(mScan, "KP_VERTICALBAR", INT2NUM(SDL_SCANCODE_KP_VERTICALBAR));
    /* @return [Integer] scancode for "KP_DBLVERTICALBAR" key */
    rb_define_const(mScan, "KP_DBLVERTICALBAR", INT2NUM(SDL_SCANCODE_KP_DBLVERTICALBAR));
    /* @return [Integer] scancode for "KP_COLON" key */
    rb_define_const(mScan, "KP_COLON", INT2NUM(SDL_SCANCODE_KP_COLON));
    /* @return [Integer] scancode for "KP_HASH" key */
    rb_define_const(mScan, "KP_HASH", INT2NUM(SDL_SCANCODE_KP_HASH));
    /* @return [Integer] scancode for "KP_SPACE" key */
    rb_define_const(mScan, "KP_SPACE", INT2NUM(SDL_SCANCODE_KP_SPACE));
    /* @return [Integer] scancode for "KP_AT" key */
    rb_define_const(mScan, "KP_AT", INT2NUM(SDL_SCANCODE_KP_AT));
    /* @return [Integer] scancode for "KP_EXCLAM" key */
    rb_define_const(mScan, "KP_EXCLAM", INT2NUM(SDL_SCANCODE_KP_EXCLAM));
    /* @return [Integer] scancode for "KP_MEMSTORE" key */
    rb_define_const(mScan, "KP_MEMSTORE", INT2NUM(SDL_SCANCODE_KP_MEMSTORE));
    /* @return [Integer] scancode for "KP_MEMRECALL" key */
    rb_define_const(mScan, "KP_MEMRECALL", INT2NUM(SDL_SCANCODE_KP_MEMRECALL));
    /* @return [Integer] scancode for "KP_MEMCLEAR" key */
    rb_define_const(mScan, "KP_MEMCLEAR", INT2NUM(SDL_SCANCODE_KP_MEMCLEAR));
    /* @return [Integer] scancode for "KP_MEMADD" key */
    rb_define_const(mScan, "KP_MEMADD", INT2NUM(SDL_SCANCODE_KP_MEMADD));
    /* @return [Integer] scancode for "KP_MEMSUBTRACT" key */
    rb_define_const(mScan, "KP_MEMSUBTRACT", INT2NUM(SDL_SCANCODE_KP_MEMSUBTRACT));
    /* @return [Integer] scancode for "KP_MEMMULTIPLY" key */
    rb_define_const(mScan, "KP_MEMMULTIPLY", INT2NUM(SDL_SCANCODE_KP_MEMMULTIPLY));
    /* @return [Integer] scancode for "KP_MEMDIVIDE" key */
    rb_define_const(mScan, "KP_MEMDIVIDE", INT2NUM(SDL_SCANCODE_KP_MEMDIVIDE));
    /* @return [Integer] scancode for "KP_PLUSMINUS" key */
    rb_define_const(mScan, "KP_PLUSMINUS", INT2NUM(SDL_SCANCODE_KP_PLUSMINUS));
    /* @return [Integer] scancode for "KP_CLEAR" key */
    rb_define_const(mScan, "KP_CLEAR", INT2NUM(SDL_SCANCODE_KP_CLEAR));
    /* @return [Integer] scancode for "KP_CLEARENTRY" key */
    rb_define_const(mScan, "KP_CLEARENTRY", INT2NUM(SDL_SCANCODE_KP_CLEARENTRY));
    /* @return [Integer] scancode for "KP_BINARY" key */
    rb_define_const(mScan, "KP_BINARY", INT2NUM(SDL_SCANCODE_KP_BINARY));
    /* @return [Integer] scancode for "KP_OCTAL" key */
    rb_define_const(mScan, "KP_OCTAL", INT2NUM(SDL_SCANCODE_KP_OCTAL));
    /* @return [Integer] scancode for "KP_DECIMAL" key */
    rb_define_const(mScan, "KP_DECIMAL", INT2NUM(SDL_SCANCODE_KP_DECIMAL));
    /* @return [Integer] scancode for "KP_HEXADECIMAL" key */
    rb_define_const(mScan, "KP_HEXADECIMAL", INT2NUM(SDL_SCANCODE_KP_HEXADECIMAL));

    /* @return [Integer] scancode for "LCTRL" key */
    rb_define_const(mScan, "LCTRL", INT2NUM(SDL_SCANCODE_LCTRL));
    /* @return [Integer] scancode for "LSHIFT" key */
    rb_define_const(mScan, "LSHIFT", INT2NUM(SDL_SCANCODE_LSHIFT));
    /* @return [Integer] scancode for "LALT" key */
    rb_define_const(mScan, "LALT", INT2NUM(SDL_SCANCODE_LALT));
    /* @return [Integer] scancode for "LGUI" key */
    rb_define_const(mScan, "LGUI", INT2NUM(SDL_SCANCODE_LGUI));
    /* @return [Integer] scancode for "RCTRL" key */
    rb_define_const(mScan, "RCTRL", INT2NUM(SDL_SCANCODE_RCTRL));
    /* @return [Integer] scancode for "RSHIFT" key */
    rb_define_const(mScan, "RSHIFT", INT2NUM(SDL_SCANCODE_RSHIFT));
    /* @return [Integer] scancode for "RALT" key */
    rb_define_const(mScan, "RALT", INT2NUM(SDL_SCANCODE_RALT));
    /* @return [Integer] scancode for "RGUI" key */
    rb_define_const(mScan, "RGUI", INT2NUM(SDL_SCANCODE_RGUI));

    /* @return [Integer] scancode for "MODE" key */
    rb_define_const(mScan, "MODE", INT2NUM(SDL_SCANCODE_MODE));

    /* @return [Integer] scancode for "AUDIONEXT" key */
    rb_define_const(mScan, "AUDIONEXT", INT2NUM(SDL_SCANCODE_AUDIONEXT));
    /* @return [Integer] scancode for "AUDIOPREV" key */
    rb_define_const(mScan, "AUDIOPREV", INT2NUM(SDL_SCANCODE_AUDIOPREV));
    /* @return [Integer] scancode for "AUDIOSTOP" key */
    rb_define_const(mScan, "AUDIOSTOP", INT2NUM(SDL_SCANCODE_AUDIOSTOP));
    /* @return [Integer] scancode for "AUDIOPLAY" key */
    rb_define_const(mScan, "AUDIOPLAY", INT2NUM(SDL_SCANCODE_AUDIOPLAY));
    /* @return [Integer] scancode for "AUDIOMUTE" key */
    rb_define_const(mScan, "AUDIOMUTE", INT2NUM(SDL_SCANCODE_AUDIOMUTE));
    /* @return [Integer] scancode for "MEDIASELECT" key */
    rb_define_const(mScan, "MEDIASELECT", INT2NUM(SDL_SCANCODE_MEDIASELECT));
    /* @return [Integer] scancode for "WWW" key */
    rb_define_const(mScan, "WWW", INT2NUM(SDL_SCANCODE_WWW));
    /* @return [Integer] scancode for "MAIL" key */
    rb_define_const(mScan, "MAIL", INT2NUM(SDL_SCANCODE_MAIL));
    /* @return [Integer] scancode for "CALCULATOR" key */
    rb_define_const(mScan, "CALCULATOR", INT2NUM(SDL_SCANCODE_CALCULATOR));
    /* @return [Integer] scancode for "COMPUTER" key */
    rb_define_const(mScan, "COMPUTER", INT2NUM(SDL_SCANCODE_COMPUTER));
    /* @return [Integer] scancode for "AC_SEARCH" key */
    rb_define_const(mScan, "AC_SEARCH", INT2NUM(SDL_SCANCODE_AC_SEARCH));
    /* @return [Integer] scancode for "AC_HOME" key */
    rb_define_const(mScan, "AC_HOME", INT2NUM(SDL_SCANCODE_AC_HOME));
    /* @return [Integer] scancode for "AC_BACK" key */
    rb_define_const(mScan, "AC_BACK", INT2NUM(SDL_SCANCODE_AC_BACK));
    /* @return [Integer] scancode for "AC_FORWARD" key */
    rb_define_const(mScan, "AC_FORWARD", INT2NUM(SDL_SCANCODE_AC_FORWARD));
    /* @return [Integer] scancode for "AC_STOP" key */
    rb_define_const(mScan, "AC_STOP", INT2NUM(SDL_SCANCODE_AC_STOP));
    /* @return [Integer] scancode for "AC_REFRESH" key */
    rb_define_const(mScan, "AC_REFRESH", INT2NUM(SDL_SCANCODE_AC_REFRESH));
    /* @return [Integer] scancode for "AC_BOOKMARKS" key */
    rb_define_const(mScan, "AC_BOOKMARKS", INT2NUM(SDL_SCANCODE_AC_BOOKMARKS));

    /* @return [Integer] scancode for "BRIGHTNESSDOWN" key */
    rb_define_const(mScan, "BRIGHTNESSDOWN", INT2NUM(SDL_SCANCODE_BRIGHTNESSDOWN));
    /* @return [Integer] scancode for "BRIGHTNESSUP" key */
    rb_define_const(mScan, "BRIGHTNESSUP", INT2NUM(SDL_SCANCODE_BRIGHTNESSUP));
    /* @return [Integer] scancode for "DISPLAYSWITCH" key */
    rb_define_const(mScan, "DISPLAYSWITCH", INT2NUM(SDL_SCANCODE_DISPLAYSWITCH));

    /* @return [Integer] scancode for "KBDILLUMTOGGLE" key */
    rb_define_const(mScan, "KBDILLUMTOGGLE", INT2NUM(SDL_SCANCODE_KBDILLUMTOGGLE));
    /* @return [Integer] scancode for "KBDILLUMDOWN" key */
    rb_define_const(mScan, "KBDILLUMDOWN", INT2NUM(SDL_SCANCODE_KBDILLUMDOWN));
    /* @return [Integer] scancode for "KBDILLUMUP" key */
    rb_define_const(mScan, "KBDILLUMUP", INT2NUM(SDL_SCANCODE_KBDILLUMUP));
    /* @return [Integer] scancode for "EJECT" key */
    rb_define_const(mScan, "EJECT", INT2NUM(SDL_SCANCODE_EJECT));
    /* @return [Integer] scancode for "SLEEP" key */
    rb_define_const(mScan, "SLEEP", INT2NUM(SDL_SCANCODE_SLEEP));

    /* @return [Integer] scancode for "APP1" key */
    rb_define_const(mScan, "APP1", INT2NUM(SDL_SCANCODE_APP1));
    /* @return [Integer] scancode for "APP2" key */
    rb_define_const(mScan, "APP2", INT2NUM(SDL_SCANCODE_APP2));

    /* @return [Integer] unused keycode */
    rb_define_const(mKey, "UNKNOWN", INT2NUM(SDLK_UNKNOWN));
    /* @return [Integer] keycode for "RETURN" key */
    rb_define_const(mKey, "RETURN", INT2NUM(SDLK_RETURN));
    /* @return [Integer] keycode for "ESCAPE" key */
    rb_define_const(mKey, "ESCAPE", INT2NUM(SDLK_ESCAPE));
    /* @return [Integer] keycode for "BACKSPACE" key */
    rb_define_const(mKey, "BACKSPACE", INT2NUM(SDLK_BACKSPACE));
    /* @return [Integer] keycode for "TAB" key */
    rb_define_const(mKey, "TAB", INT2NUM(SDLK_TAB));
    /* @return [Integer] keycode for "SPACE" key */
    rb_define_const(mKey, "SPACE", INT2NUM(SDLK_SPACE));
    /* @return [Integer] keycode for "EXCLAIM" key */
    rb_define_const(mKey, "EXCLAIM", INT2NUM(SDLK_EXCLAIM));
    /* @return [Integer] keycode for "QUOTEDBL" key */
    rb_define_const(mKey, "QUOTEDBL", INT2NUM(SDLK_QUOTEDBL));
    /* @return [Integer] keycode for "HASH" key */
    rb_define_const(mKey, "HASH", INT2NUM(SDLK_HASH));
    /* @return [Integer] keycode for "PERCENT" key */
    rb_define_const(mKey, "PERCENT", INT2NUM(SDLK_PERCENT));
    /* @return [Integer] keycode for "DOLLAR" key */
    rb_define_const(mKey, "DOLLAR", INT2NUM(SDLK_DOLLAR));
    /* @return [Integer] keycode for "AMPERSAND" key */
    rb_define_const(mKey, "AMPERSAND", INT2NUM(SDLK_AMPERSAND));
    /* @return [Integer] keycode for "QUOTE" key */
    rb_define_const(mKey, "QUOTE", INT2NUM(SDLK_QUOTE));
    /* @return [Integer] keycode for "LEFTPAREN" key */
    rb_define_const(mKey, "LEFTPAREN", INT2NUM(SDLK_LEFTPAREN));
    /* @return [Integer] keycode for "RIGHTPAREN" key */
    rb_define_const(mKey, "RIGHTPAREN", INT2NUM(SDLK_RIGHTPAREN));
    /* @return [Integer] keycode for "ASTERISK" key */
    rb_define_const(mKey, "ASTERISK", INT2NUM(SDLK_ASTERISK));
    /* @return [Integer] keycode for "PLUS" key */
    rb_define_const(mKey, "PLUS", INT2NUM(SDLK_PLUS));
    /* @return [Integer] keycode for "COMMA" key */
    rb_define_const(mKey, "COMMA", INT2NUM(SDLK_COMMA));
    /* @return [Integer] keycode for "MINUS" key */
    rb_define_const(mKey, "MINUS", INT2NUM(SDLK_MINUS));
    /* @return [Integer] keycode for "PERIOD" key */
    rb_define_const(mKey, "PERIOD", INT2NUM(SDLK_PERIOD));
    /* @return [Integer] keycode for "SLASH" key */
    rb_define_const(mKey, "SLASH", INT2NUM(SDLK_SLASH));
    /* @return [Integer] keycode for number key "0" (not on keypad) */
    rb_define_const(mKey, "K0", INT2NUM(SDLK_0));
    /* @return [Integer] keycode for number key "1" (not on keypad) */
    rb_define_const(mKey, "K1", INT2NUM(SDLK_1));
    /* @return [Integer] keycode for number key "2" (not on keypad) */
    rb_define_const(mKey, "K2", INT2NUM(SDLK_2));
    /* @return [Integer] keycode for number key "3" (not on keypad) */
    rb_define_const(mKey, "K3", INT2NUM(SDLK_3));
    /* @return [Integer] keycode for number key "4" (not on keypad) */
    rb_define_const(mKey, "K4", INT2NUM(SDLK_4));
    /* @return [Integer] keycode for number key "5" (not on keypad) */
    rb_define_const(mKey, "K5", INT2NUM(SDLK_5));
    /* @return [Integer] keycode for number key "6" (not on keypad) */
    rb_define_const(mKey, "K6", INT2NUM(SDLK_6));
    /* @return [Integer] keycode for number key "7" (not on keypad) */
    rb_define_const(mKey, "K7", INT2NUM(SDLK_7));
    /* @return [Integer] keycode for number key "8" (not on keypad) */
    rb_define_const(mKey, "K8", INT2NUM(SDLK_8));
    /* @return [Integer] keycode for number key "9" (not on keypad) */
    rb_define_const(mKey, "K9", INT2NUM(SDLK_9));
    /* @return [Integer] keycode for "COLON" key */
    rb_define_const(mKey, "COLON", INT2NUM(SDLK_COLON));
    /* @return [Integer] keycode for "SEMICOLON" key */
    rb_define_const(mKey, "SEMICOLON", INT2NUM(SDLK_SEMICOLON));
    /* @return [Integer] keycode for "LESS" key */
    rb_define_const(mKey, "LESS", INT2NUM(SDLK_LESS));
    /* @return [Integer] keycode for "EQUALS" key */
    rb_define_const(mKey, "EQUALS", INT2NUM(SDLK_EQUALS));
    /* @return [Integer] keycode for "GREATER" key */
    rb_define_const(mKey, "GREATER", INT2NUM(SDLK_GREATER));
    /* @return [Integer] keycode for "QUESTION" key */
    rb_define_const(mKey, "QUESTION", INT2NUM(SDLK_QUESTION));
    /* @return [Integer] keycode for "AT" key */
    rb_define_const(mKey, "AT", INT2NUM(SDLK_AT));
    /*
       Skip uppercase letters
     */
    /* @return [Integer] keycode for "LEFTBRACKET" key */
    rb_define_const(mKey, "LEFTBRACKET", INT2NUM(SDLK_LEFTBRACKET));
    /* @return [Integer] keycode for "BACKSLASH" key */
    rb_define_const(mKey, "BACKSLASH", INT2NUM(SDLK_BACKSLASH));
    /* @return [Integer] keycode for "RIGHTBRACKET" key */
    rb_define_const(mKey, "RIGHTBRACKET", INT2NUM(SDLK_RIGHTBRACKET));
    /* @return [Integer] keycode for "CARET" key */
    rb_define_const(mKey, "CARET", INT2NUM(SDLK_CARET));
    /* @return [Integer] keycode for "UNDERSCORE" key */
    rb_define_const(mKey, "UNDERSCORE", INT2NUM(SDLK_UNDERSCORE));
    /* @return [Integer] keycode for "BACKQUOTE" key */
    rb_define_const(mKey, "BACKQUOTE", INT2NUM(SDLK_BACKQUOTE));
    
    /* @return [Integer] keycode for alphabet key "a" */
    rb_define_const(mKey, "A", INT2NUM(SDLK_a));
    /* @return [Integer] keycode for alphabet key "b" */
    rb_define_const(mKey, "B", INT2NUM(SDLK_b));
    /* @return [Integer] keycode for alphabet key "c" */
    rb_define_const(mKey, "C", INT2NUM(SDLK_c));
    /* @return [Integer] keycode for alphabet key "d" */
    rb_define_const(mKey, "D", INT2NUM(SDLK_d));
    /* @return [Integer] keycode for alphabet key "e" */
    rb_define_const(mKey, "E", INT2NUM(SDLK_e));
    /* @return [Integer] keycode for alphabet key "f" */
    rb_define_const(mKey, "F", INT2NUM(SDLK_f));
    /* @return [Integer] keycode for alphabet key "g" */
    rb_define_const(mKey, "G", INT2NUM(SDLK_g));
    /* @return [Integer] keycode for alphabet key "h" */
    rb_define_const(mKey, "H", INT2NUM(SDLK_h));
    /* @return [Integer] keycode for alphabet key "i" */
    rb_define_const(mKey, "I", INT2NUM(SDLK_i));
    /* @return [Integer] keycode for alphabet key "j" */
    rb_define_const(mKey, "J", INT2NUM(SDLK_j));
    /* @return [Integer] keycode for alphabet key "k" */
    rb_define_const(mKey, "K", INT2NUM(SDLK_k));
    /* @return [Integer] keycode for alphabet key "l" */
    rb_define_const(mKey, "L", INT2NUM(SDLK_l));
    /* @return [Integer] keycode for alphabet key "m" */
    rb_define_const(mKey, "M", INT2NUM(SDLK_m));
    /* @return [Integer] keycode for alphabet key "n" */
    rb_define_const(mKey, "N", INT2NUM(SDLK_n));
    /* @return [Integer] keycode for alphabet key "o" */
    rb_define_const(mKey, "O", INT2NUM(SDLK_o));
    /* @return [Integer] keycode for alphabet key "p" */
    rb_define_const(mKey, "P", INT2NUM(SDLK_p));
    /* @return [Integer] keycode for alphabet key "q" */
    rb_define_const(mKey, "Q", INT2NUM(SDLK_q));
    /* @return [Integer] keycode for alphabet key "r" */
    rb_define_const(mKey, "R", INT2NUM(SDLK_r));
    /* @return [Integer] keycode for alphabet key "s" */
    rb_define_const(mKey, "S", INT2NUM(SDLK_s));
    /* @return [Integer] keycode for alphabet key "t" */
    rb_define_const(mKey, "T", INT2NUM(SDLK_t));
    /* @return [Integer] keycode for alphabet key "u" */
    rb_define_const(mKey, "U", INT2NUM(SDLK_u));
    /* @return [Integer] keycode for alphabet key "v" */
    rb_define_const(mKey, "V", INT2NUM(SDLK_v));
    /* @return [Integer] keycode for alphabet key "w" */
    rb_define_const(mKey, "W", INT2NUM(SDLK_w));
    /* @return [Integer] keycode for alphabet key "x" */
    rb_define_const(mKey, "X", INT2NUM(SDLK_x));
    /* @return [Integer] keycode for alphabet key "y" */
    rb_define_const(mKey, "Y", INT2NUM(SDLK_y));
    /* @return [Integer] keycode for alphabet key "z" */
    rb_define_const(mKey, "Z", INT2NUM(SDLK_z));

    /* @return [Integer] keycode for "CAPSLOCK" key */
    rb_define_const(mKey, "CAPSLOCK", INT2NUM(SDLK_CAPSLOCK));

    /* @return [Integer] keycode for "F1" key */
    rb_define_const(mKey, "F1", INT2NUM(SDLK_F1));
    /* @return [Integer] keycode for "F2" key */
    rb_define_const(mKey, "F2", INT2NUM(SDLK_F2));
    /* @return [Integer] keycode for "F3" key */
    rb_define_const(mKey, "F3", INT2NUM(SDLK_F3));
    /* @return [Integer] keycode for "F4" key */
    rb_define_const(mKey, "F4", INT2NUM(SDLK_F4));
    /* @return [Integer] keycode for "F5" key */
    rb_define_const(mKey, "F5", INT2NUM(SDLK_F5));
    /* @return [Integer] keycode for "F6" key */
    rb_define_const(mKey, "F6", INT2NUM(SDLK_F6));
    /* @return [Integer] keycode for "F7" key */
    rb_define_const(mKey, "F7", INT2NUM(SDLK_F7));
    /* @return [Integer] keycode for "F8" key */
    rb_define_const(mKey, "F8", INT2NUM(SDLK_F8));
    /* @return [Integer] keycode for "F9" key */
    rb_define_const(mKey, "F9", INT2NUM(SDLK_F9));
    /* @return [Integer] keycode for "F10" key */
    rb_define_const(mKey, "F10", INT2NUM(SDLK_F10));
    /* @return [Integer] keycode for "F11" key */
    rb_define_const(mKey, "F11", INT2NUM(SDLK_F11));
    /* @return [Integer] keycode for "F12" key */
    rb_define_const(mKey, "F12", INT2NUM(SDLK_F12));

    /* @return [Integer] keycode for "PRINTSCREEN" key */
    rb_define_const(mKey, "PRINTSCREEN", INT2NUM(SDLK_PRINTSCREEN));
    /* @return [Integer] keycode for "SCROLLLOCK" key */
    rb_define_const(mKey, "SCROLLLOCK", INT2NUM(SDLK_SCROLLLOCK));
    /* @return [Integer] keycode for "PAUSE" key */
    rb_define_const(mKey, "PAUSE", INT2NUM(SDLK_PAUSE));
    /* @return [Integer] keycode for "INSERT" key */
    rb_define_const(mKey, "INSERT", INT2NUM(SDLK_INSERT));
    /* @return [Integer] keycode for "HOME" key */
    rb_define_const(mKey, "HOME", INT2NUM(SDLK_HOME));
    /* @return [Integer] keycode for "PAGEUP" key */
    rb_define_const(mKey, "PAGEUP", INT2NUM(SDLK_PAGEUP));
    /* @return [Integer] keycode for "DELETE" key */
    rb_define_const(mKey, "DELETE", INT2NUM(SDLK_DELETE));
    /* @return [Integer] keycode for "END" key */
    rb_define_const(mKey, "END", INT2NUM(SDLK_END));
    /* @return [Integer] keycode for "PAGEDOWN" key */
    rb_define_const(mKey, "PAGEDOWN", INT2NUM(SDLK_PAGEDOWN));
    /* @return [Integer] keycode for "RIGHT" key */
    rb_define_const(mKey, "RIGHT", INT2NUM(SDLK_RIGHT));
    /* @return [Integer] keycode for "LEFT" key */
    rb_define_const(mKey, "LEFT", INT2NUM(SDLK_LEFT));
    /* @return [Integer] keycode for "DOWN" key */
    rb_define_const(mKey, "DOWN", INT2NUM(SDLK_DOWN));
    /* @return [Integer] keycode for "UP" key */
    rb_define_const(mKey, "UP", INT2NUM(SDLK_UP));

    /* @return [Integer] keycode for "NUMLOCKCLEAR" key */
    rb_define_const(mKey, "NUMLOCKCLEAR", INT2NUM(SDLK_NUMLOCKCLEAR));
    /* @return [Integer] keycode for "KP_DIVIDE" key */
    rb_define_const(mKey, "KP_DIVIDE", INT2NUM(SDLK_KP_DIVIDE));
    /* @return [Integer] keycode for "KP_MULTIPLY" key */
    rb_define_const(mKey, "KP_MULTIPLY", INT2NUM(SDLK_KP_MULTIPLY));
    /* @return [Integer] keycode for "KP_MINUS" key */
    rb_define_const(mKey, "KP_MINUS", INT2NUM(SDLK_KP_MINUS));
    /* @return [Integer] keycode for "KP_PLUS" key */
    rb_define_const(mKey, "KP_PLUS", INT2NUM(SDLK_KP_PLUS));
    /* @return [Integer] keycode for "KP_ENTER" key */
    rb_define_const(mKey, "KP_ENTER", INT2NUM(SDLK_KP_ENTER));
    /* @return [Integer] keycode for "KP_1" key */
    rb_define_const(mKey, "KP_1", INT2NUM(SDLK_KP_1));
    /* @return [Integer] keycode for "KP_2" key */
    rb_define_const(mKey, "KP_2", INT2NUM(SDLK_KP_2));
    /* @return [Integer] keycode for "KP_3" key */
    rb_define_const(mKey, "KP_3", INT2NUM(SDLK_KP_3));
    /* @return [Integer] keycode for "KP_4" key */
    rb_define_const(mKey, "KP_4", INT2NUM(SDLK_KP_4));
    /* @return [Integer] keycode for "KP_5" key */
    rb_define_const(mKey, "KP_5", INT2NUM(SDLK_KP_5));
    /* @return [Integer] keycode for "KP_6" key */
    rb_define_const(mKey, "KP_6", INT2NUM(SDLK_KP_6));
    /* @return [Integer] keycode for "KP_7" key */
    rb_define_const(mKey, "KP_7", INT2NUM(SDLK_KP_7));
    /* @return [Integer] keycode for "KP_8" key */
    rb_define_const(mKey, "KP_8", INT2NUM(SDLK_KP_8));
    /* @return [Integer] keycode for "KP_9" key */
    rb_define_const(mKey, "KP_9", INT2NUM(SDLK_KP_9));
    /* @return [Integer] keycode for "KP_0" key */
    rb_define_const(mKey, "KP_0", INT2NUM(SDLK_KP_0));
    /* @return [Integer] keycode for "KP_PERIOD" key */
    rb_define_const(mKey, "KP_PERIOD", INT2NUM(SDLK_KP_PERIOD));

    /* @return [Integer] keycode for "APPLICATION" key */
    rb_define_const(mKey, "APPLICATION", INT2NUM(SDLK_APPLICATION));
    /* @return [Integer] keycode for "POWER" key */
    rb_define_const(mKey, "POWER", INT2NUM(SDLK_POWER));
    /* @return [Integer] keycode for "KP_EQUALS" key */
    rb_define_const(mKey, "KP_EQUALS", INT2NUM(SDLK_KP_EQUALS));
    /* @return [Integer] keycode for "F13" key */
    rb_define_const(mKey, "F13", INT2NUM(SDLK_F13));
    /* @return [Integer] keycode for "F14" key */
    rb_define_const(mKey, "F14", INT2NUM(SDLK_F14));
    /* @return [Integer] keycode for "F15" key */
    rb_define_const(mKey, "F15", INT2NUM(SDLK_F15));
    /* @return [Integer] keycode for "F16" key */
    rb_define_const(mKey, "F16", INT2NUM(SDLK_F16));
    /* @return [Integer] keycode for "F17" key */
    rb_define_const(mKey, "F17", INT2NUM(SDLK_F17));
    /* @return [Integer] keycode for "F18" key */
    rb_define_const(mKey, "F18", INT2NUM(SDLK_F18));
    /* @return [Integer] keycode for "F19" key */
    rb_define_const(mKey, "F19", INT2NUM(SDLK_F19));
    /* @return [Integer] keycode for "F20" key */
    rb_define_const(mKey, "F20", INT2NUM(SDLK_F20));
    /* @return [Integer] keycode for "F21" key */
    rb_define_const(mKey, "F21", INT2NUM(SDLK_F21));
    /* @return [Integer] keycode for "F22" key */
    rb_define_const(mKey, "F22", INT2NUM(SDLK_F22));
    /* @return [Integer] keycode for "F23" key */
    rb_define_const(mKey, "F23", INT2NUM(SDLK_F23));
    /* @return [Integer] keycode for "F24" key */
    rb_define_const(mKey, "F24", INT2NUM(SDLK_F24));
    /* @return [Integer] keycode for "EXECUTE" key */
    rb_define_const(mKey, "EXECUTE", INT2NUM(SDLK_EXECUTE));
    /* @return [Integer] keycode for "HELP" key */
    rb_define_const(mKey, "HELP", INT2NUM(SDLK_HELP));
    /* @return [Integer] keycode for "MENU" key */
    rb_define_const(mKey, "MENU", INT2NUM(SDLK_MENU));
    /* @return [Integer] keycode for "SELECT" key */
    rb_define_const(mKey, "SELECT", INT2NUM(SDLK_SELECT));
    /* @return [Integer] keycode for "STOP" key */
    rb_define_const(mKey, "STOP", INT2NUM(SDLK_STOP));
    /* @return [Integer] keycode for "AGAIN" key */
    rb_define_const(mKey, "AGAIN", INT2NUM(SDLK_AGAIN));
    /* @return [Integer] keycode for "UNDO" key */
    rb_define_const(mKey, "UNDO", INT2NUM(SDLK_UNDO));
    /* @return [Integer] keycode for "CUT" key */
    rb_define_const(mKey, "CUT", INT2NUM(SDLK_CUT));
    /* @return [Integer] keycode for "COPY" key */
    rb_define_const(mKey, "COPY", INT2NUM(SDLK_COPY));
    /* @return [Integer] keycode for "PASTE" key */
    rb_define_const(mKey, "PASTE", INT2NUM(SDLK_PASTE));
    /* @return [Integer] keycode for "FIND" key */
    rb_define_const(mKey, "FIND", INT2NUM(SDLK_FIND));
    /* @return [Integer] keycode for "MUTE" key */
    rb_define_const(mKey, "MUTE", INT2NUM(SDLK_MUTE));
    /* @return [Integer] keycode for "VOLUMEUP" key */
    rb_define_const(mKey, "VOLUMEUP", INT2NUM(SDLK_VOLUMEUP));
    /* @return [Integer] keycode for "VOLUMEDOWN" key */
    rb_define_const(mKey, "VOLUMEDOWN", INT2NUM(SDLK_VOLUMEDOWN));
    /* @return [Integer] keycode for "KP_COMMA" key */
    rb_define_const(mKey, "KP_COMMA", INT2NUM(SDLK_KP_COMMA));
    /* @return [Integer] keycode for "KP_EQUALSAS400" key */
    rb_define_const(mKey, "KP_EQUALSAS400", INT2NUM(SDLK_KP_EQUALSAS400));
    
    /* @return [Integer] keycode for "ALTERASE" key */
    rb_define_const(mKey, "ALTERASE", INT2NUM(SDLK_ALTERASE));
    /* @return [Integer] keycode for "SYSREQ" key */
    rb_define_const(mKey, "SYSREQ", INT2NUM(SDLK_SYSREQ));
    /* @return [Integer] keycode for "CANCEL" key */
    rb_define_const(mKey, "CANCEL", INT2NUM(SDLK_CANCEL));
    /* @return [Integer] keycode for "CLEAR" key */
    rb_define_const(mKey, "CLEAR", INT2NUM(SDLK_CLEAR));
    /* @return [Integer] keycode for "PRIOR" key */
    rb_define_const(mKey, "PRIOR", INT2NUM(SDLK_PRIOR));
    /* @return [Integer] keycode for "RETURN2" key */
    rb_define_const(mKey, "RETURN2", INT2NUM(SDLK_RETURN2));
    /* @return [Integer] keycode for "SEPARATOR" key */
    rb_define_const(mKey, "SEPARATOR", INT2NUM(SDLK_SEPARATOR));
    /* @return [Integer] keycode for "OUT" key */
    rb_define_const(mKey, "OUT", INT2NUM(SDLK_OUT));
    /* @return [Integer] keycode for "OPER" key */
    rb_define_const(mKey, "OPER", INT2NUM(SDLK_OPER));
    /* @return [Integer] keycode for "CLEARAGAIN" key */
    rb_define_const(mKey, "CLEARAGAIN", INT2NUM(SDLK_CLEARAGAIN));
    /* @return [Integer] keycode for "CRSEL" key */
    rb_define_const(mKey, "CRSEL", INT2NUM(SDLK_CRSEL));
    /* @return [Integer] keycode for "EXSEL" key */
    rb_define_const(mKey, "EXSEL", INT2NUM(SDLK_EXSEL));

    /* @return [Integer] keycode for "KP_00" key */
    rb_define_const(mKey, "KP_00", INT2NUM(SDLK_KP_00));
    /* @return [Integer] keycode for "KP_000" key */
    rb_define_const(mKey, "KP_000", INT2NUM(SDLK_KP_000));
    /* @return [Integer] keycode for "THOUSANDSSEPARATOR" key */
    rb_define_const(mKey, "THOUSANDSSEPARATOR", INT2NUM(SDLK_THOUSANDSSEPARATOR));

    /* @return [Integer] keycode for "DECIMALSEPARATOR" key */
    rb_define_const(mKey, "DECIMALSEPARATOR", INT2NUM(SDLK_DECIMALSEPARATOR));

    /* @return [Integer] keycode for "CURRENCYUNIT" key */
    rb_define_const(mKey, "CURRENCYUNIT", INT2NUM(SDLK_CURRENCYUNIT));
    /* @return [Integer] keycode for "CURRENCYSUBUNIT" key */
    rb_define_const(mKey, "CURRENCYSUBUNIT", INT2NUM(SDLK_CURRENCYSUBUNIT));
    /* @return [Integer] keycode for "KP_LEFTPAREN" key */
    rb_define_const(mKey, "KP_LEFTPAREN", INT2NUM(SDLK_KP_LEFTPAREN));
    /* @return [Integer] keycode for "KP_RIGHTPAREN" key */
    rb_define_const(mKey, "KP_RIGHTPAREN", INT2NUM(SDLK_KP_RIGHTPAREN));
    /* @return [Integer] keycode for "KP_LEFTBRACE" key */
    rb_define_const(mKey, "KP_LEFTBRACE", INT2NUM(SDLK_KP_LEFTBRACE));
    /* @return [Integer] keycode for "KP_RIGHTBRACE" key */
    rb_define_const(mKey, "KP_RIGHTBRACE", INT2NUM(SDLK_KP_RIGHTBRACE));
    /* @return [Integer] keycode for "KP_TAB" key */
    rb_define_const(mKey, "KP_TAB", INT2NUM(SDLK_KP_TAB));
    /* @return [Integer] keycode for "KP_BACKSPACE" key */
    rb_define_const(mKey, "KP_BACKSPACE", INT2NUM(SDLK_KP_BACKSPACE));
    /* @return [Integer] keycode for "KP_A" key */
    rb_define_const(mKey, "KP_A", INT2NUM(SDLK_KP_A));
    /* @return [Integer] keycode for "KP_B" key */
    rb_define_const(mKey, "KP_B", INT2NUM(SDLK_KP_B));
    /* @return [Integer] keycode for "KP_C" key */
    rb_define_const(mKey, "KP_C", INT2NUM(SDLK_KP_C));
    /* @return [Integer] keycode for "KP_D" key */
    rb_define_const(mKey, "KP_D", INT2NUM(SDLK_KP_D));
    /* @return [Integer] keycode for "KP_E" key */
    rb_define_const(mKey, "KP_E", INT2NUM(SDLK_KP_E));
    /* @return [Integer] keycode for "KP_F" key */
    rb_define_const(mKey, "KP_F", INT2NUM(SDLK_KP_F));
    /* @return [Integer] keycode for "KP_XOR" key */
    rb_define_const(mKey, "KP_XOR", INT2NUM(SDLK_KP_XOR));
    /* @return [Integer] keycode for "KP_POWER" key */
    rb_define_const(mKey, "KP_POWER", INT2NUM(SDLK_KP_POWER));
    /* @return [Integer] keycode for "KP_PERCENT" key */
    rb_define_const(mKey, "KP_PERCENT", INT2NUM(SDLK_KP_PERCENT));
    /* @return [Integer] keycode for "KP_LESS" key */
    rb_define_const(mKey, "KP_LESS", INT2NUM(SDLK_KP_LESS));
    /* @return [Integer] keycode for "KP_GREATER" key */
    rb_define_const(mKey, "KP_GREATER", INT2NUM(SDLK_KP_GREATER));
    /* @return [Integer] keycode for "KP_AMPERSAND" key */
    rb_define_const(mKey, "KP_AMPERSAND", INT2NUM(SDLK_KP_AMPERSAND));
    /* @return [Integer] keycode for "KP_DBLAMPERSAND" key */
    rb_define_const(mKey, "KP_DBLAMPERSAND", INT2NUM(SDLK_KP_DBLAMPERSAND));
    /* @return [Integer] keycode for "KP_VERTICALBAR" key */
    rb_define_const(mKey, "KP_VERTICALBAR", INT2NUM(SDLK_KP_VERTICALBAR));
    /* @return [Integer] keycode for "KP_DBLVERTICALBAR" key */
    rb_define_const(mKey, "KP_DBLVERTICALBAR", INT2NUM(SDLK_KP_DBLVERTICALBAR));
    /* @return [Integer] keycode for "KP_COLON" key */
    rb_define_const(mKey, "KP_COLON", INT2NUM(SDLK_KP_COLON));
    /* @return [Integer] keycode for "KP_HASH" key */
    rb_define_const(mKey, "KP_HASH", INT2NUM(SDLK_KP_HASH));
    /* @return [Integer] keycode for "KP_SPACE" key */
    rb_define_const(mKey, "KP_SPACE", INT2NUM(SDLK_KP_SPACE));
    /* @return [Integer] keycode for "KP_AT" key */
    rb_define_const(mKey, "KP_AT", INT2NUM(SDLK_KP_AT));
    /* @return [Integer] keycode for "KP_EXCLAM" key */
    rb_define_const(mKey, "KP_EXCLAM", INT2NUM(SDLK_KP_EXCLAM));
    /* @return [Integer] keycode for "KP_MEMSTORE" key */
    rb_define_const(mKey, "KP_MEMSTORE", INT2NUM(SDLK_KP_MEMSTORE));
    /* @return [Integer] keycode for "KP_MEMRECALL" key */
    rb_define_const(mKey, "KP_MEMRECALL", INT2NUM(SDLK_KP_MEMRECALL));
    /* @return [Integer] keycode for "KP_MEMCLEAR" key */
    rb_define_const(mKey, "KP_MEMCLEAR", INT2NUM(SDLK_KP_MEMCLEAR));
    /* @return [Integer] keycode for "KP_MEMADD" key */
    rb_define_const(mKey, "KP_MEMADD", INT2NUM(SDLK_KP_MEMADD));
    /* @return [Integer] keycode for "KP_MEMSUBTRACT" key */
    rb_define_const(mKey, "KP_MEMSUBTRACT", INT2NUM(SDLK_KP_MEMSUBTRACT));
    /* @return [Integer] keycode for "KP_MEMMULTIPLY" key */
    rb_define_const(mKey, "KP_MEMMULTIPLY", INT2NUM(SDLK_KP_MEMMULTIPLY));
    /* @return [Integer] keycode for "KP_MEMDIVIDE" key */
    rb_define_const(mKey, "KP_MEMDIVIDE", INT2NUM(SDLK_KP_MEMDIVIDE));
    /* @return [Integer] keycode for "KP_PLUSMINUS" key */
    rb_define_const(mKey, "KP_PLUSMINUS", INT2NUM(SDLK_KP_PLUSMINUS));
    /* @return [Integer] keycode for "KP_CLEAR" key */
    rb_define_const(mKey, "KP_CLEAR", INT2NUM(SDLK_KP_CLEAR));
    /* @return [Integer] keycode for "KP_CLEARENTRY" key */
    rb_define_const(mKey, "KP_CLEARENTRY", INT2NUM(SDLK_KP_CLEARENTRY));
    /* @return [Integer] keycode for "KP_BINARY" key */
    rb_define_const(mKey, "KP_BINARY", INT2NUM(SDLK_KP_BINARY));
    /* @return [Integer] keycode for "KP_OCTAL" key */
    rb_define_const(mKey, "KP_OCTAL", INT2NUM(SDLK_KP_OCTAL));
    /* @return [Integer] keycode for "KP_DECIMAL" key */
    rb_define_const(mKey, "KP_DECIMAL", INT2NUM(SDLK_KP_DECIMAL));
    /* @return [Integer] keycode for "KP_HEXADECIMAL" key */
    rb_define_const(mKey, "KP_HEXADECIMAL", INT2NUM(SDLK_KP_HEXADECIMAL));

    /* @return [Integer] keycode for "LCTRL" key */
    rb_define_const(mKey, "LCTRL", INT2NUM(SDLK_LCTRL));
    /* @return [Integer] keycode for "LSHIFT" key */
    rb_define_const(mKey, "LSHIFT", INT2NUM(SDLK_LSHIFT));
    /* @return [Integer] keycode for "LALT" key */
    rb_define_const(mKey, "LALT", INT2NUM(SDLK_LALT));
    /* @return [Integer] keycode for "LGUI" key */
    rb_define_const(mKey, "LGUI", INT2NUM(SDLK_LGUI));
    /* @return [Integer] keycode for "RCTRL" key */
    rb_define_const(mKey, "RCTRL", INT2NUM(SDLK_RCTRL));
    /* @return [Integer] keycode for "RSHIFT" key */
    rb_define_const(mKey, "RSHIFT", INT2NUM(SDLK_RSHIFT));
    /* @return [Integer] keycode for "RALT" key */
    rb_define_const(mKey, "RALT", INT2NUM(SDLK_RALT));
    /* @return [Integer] keycode for "RGUI" key */
    rb_define_const(mKey, "RGUI", INT2NUM(SDLK_RGUI));

    /* @return [Integer] keycode for "MODE" key */
    rb_define_const(mKey, "MODE", INT2NUM(SDLK_MODE));

    /* @return [Integer] keycode for "AUDIONEXT" key */
    rb_define_const(mKey, "AUDIONEXT", INT2NUM(SDLK_AUDIONEXT));
    /* @return [Integer] keycode for "AUDIOPREV" key */
    rb_define_const(mKey, "AUDIOPREV", INT2NUM(SDLK_AUDIOPREV));
    /* @return [Integer] keycode for "AUDIOSTOP" key */
    rb_define_const(mKey, "AUDIOSTOP", INT2NUM(SDLK_AUDIOSTOP));
    /* @return [Integer] keycode for "AUDIOPLAY" key */
    rb_define_const(mKey, "AUDIOPLAY", INT2NUM(SDLK_AUDIOPLAY));
    /* @return [Integer] keycode for "AUDIOMUTE" key */
    rb_define_const(mKey, "AUDIOMUTE", INT2NUM(SDLK_AUDIOMUTE));
    /* @return [Integer] keycode for "MEDIASELECT" key */
    rb_define_const(mKey, "MEDIASELECT", INT2NUM(SDLK_MEDIASELECT));
    /* @return [Integer] keycode for "WWW" key */
    rb_define_const(mKey, "WWW", INT2NUM(SDLK_WWW));
    /* @return [Integer] keycode for "MAIL" key */
    rb_define_const(mKey, "MAIL", INT2NUM(SDLK_MAIL));
    /* @return [Integer] keycode for "CALCULATOR" key */
    rb_define_const(mKey, "CALCULATOR", INT2NUM(SDLK_CALCULATOR));
    /* @return [Integer] keycode for "COMPUTER" key */
    rb_define_const(mKey, "COMPUTER", INT2NUM(SDLK_COMPUTER));
    /* @return [Integer] keycode for "AC_SEARCH" key */
    rb_define_const(mKey, "AC_SEARCH", INT2NUM(SDLK_AC_SEARCH));
    /* @return [Integer] keycode for "AC_HOME" key */
    rb_define_const(mKey, "AC_HOME", INT2NUM(SDLK_AC_HOME));
    /* @return [Integer] keycode for "AC_BACK" key */
    rb_define_const(mKey, "AC_BACK", INT2NUM(SDLK_AC_BACK));
    /* @return [Integer] keycode for "AC_FORWARD" key */
    rb_define_const(mKey, "AC_FORWARD", INT2NUM(SDLK_AC_FORWARD));
    /* @return [Integer] keycode for "AC_STOP" key */
    rb_define_const(mKey, "AC_STOP", INT2NUM(SDLK_AC_STOP));
    /* @return [Integer] keycode for "AC_REFRESH" key */
    rb_define_const(mKey, "AC_REFRESH", INT2NUM(SDLK_AC_REFRESH));
    /* @return [Integer] keycode for "AC_BOOKMARKS" key */
    rb_define_const(mKey, "AC_BOOKMARKS", INT2NUM(SDLK_AC_BOOKMARKS));

    /* @return [Integer] keycode for "BRIGHTNESSDOWN" key */
    rb_define_const(mKey, "BRIGHTNESSDOWN", INT2NUM(SDLK_BRIGHTNESSDOWN));

    /* @return [Integer] keycode for "BRIGHTNESSUP" key */
    rb_define_const(mKey, "BRIGHTNESSUP", INT2NUM(SDLK_BRIGHTNESSUP));
    /* @return [Integer] keycode for "DISPLAYSWITCH" key */
    rb_define_const(mKey, "DISPLAYSWITCH", INT2NUM(SDLK_DISPLAYSWITCH));
    /* @return [Integer] keycode for "KBDILLUMTOGGLE" key */
    rb_define_const(mKey, "KBDILLUMTOGGLE", INT2NUM(SDLK_KBDILLUMTOGGLE));

    /* @return [Integer] keycode for "KBDILLUMDOWN" key */
    rb_define_const(mKey, "KBDILLUMDOWN", INT2NUM(SDLK_KBDILLUMDOWN));
    /* @return [Integer] keycode for "KBDILLUMUP" key */
    rb_define_const(mKey, "KBDILLUMUP", INT2NUM(SDLK_KBDILLUMUP));
    /* @return [Integer] keycode for "EJECT" key */
    rb_define_const(mKey, "EJECT", INT2NUM(SDLK_EJECT));
    /* @return [Integer] keycode for "SLEEP" key */
    rb_define_const(mKey, "SLEEP", INT2NUM(SDLK_SLEEP));

    /* 0 (no modifier is applicable) */
    rb_define_const(mMod, "NONE", INT2NUM(KMOD_NONE));
    /* the modifier key bit mask for the left shift key */
    rb_define_const(mMod, "LSHIFT", INT2NUM(KMOD_LSHIFT));
    /* the modifier key bit mask for the right shift key */
    rb_define_const(mMod, "RSHIFT", INT2NUM(KMOD_RSHIFT));
    /* the modifier key bit mask for the left control key */
    rb_define_const(mMod, "LCTRL", INT2NUM(KMOD_LCTRL));
    /* the modifier key bit mask for the right control key */
    rb_define_const(mMod, "RCTRL", INT2NUM(KMOD_RCTRL));
    /* the modifier key bit mask for the left alt key */
    rb_define_const(mMod, "LALT", INT2NUM(KMOD_LALT));
    /* the modifier key bit mask for the right alt key */
    rb_define_const(mMod, "RALT", INT2NUM(KMOD_RALT));
    /* the modifier key bit mask for the left GUI key (often the window key) */
    rb_define_const(mMod, "LGUI", INT2NUM(KMOD_LGUI));
    /* the modifier key bit mask for the right GUI key (often the window key) */
    rb_define_const(mMod, "RGUI", INT2NUM(KMOD_RGUI));
    /* the modifier key bit mask for the numlock key */
    rb_define_const(mMod, "NUM", INT2NUM(KMOD_NUM));
    /* the modifier key bit mask for the capslock key */
    rb_define_const(mMod, "CAPS", INT2NUM(KMOD_CAPS));
    /* the modifier key bit mask for the mode key (AltGr) */
    rb_define_const(mMod, "MODE", INT2NUM(KMOD_MODE));
    /* the modifier key bit mask for the left and right control key */
    rb_define_const(mMod, "CTRL", INT2NUM(KMOD_CTRL));
    /* the modifier key bit mask for the left and right shift key */
    rb_define_const(mMod, "SHIFT", INT2NUM(KMOD_SHIFT));
    /* the modifier key bit mask for the left and right alt key */
    rb_define_const(mMod, "ALT", INT2NUM(KMOD_ALT));
    /* the modifier key bit mask for the left and right GUI key */
    rb_define_const(mMod, "GUI", INT2NUM(KMOD_GUI));
    /* reserved for future use */
    rb_define_const(mMod, "RESERVED", INT2NUM(KMOD_RESERVED));
}
