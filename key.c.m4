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
define(`DEFINE_SCANCODE',`ifelse(`$#',`2',`$2
    ',`/$8* @return [Integer] scancode for "$1" key *$8/
    ')rb_define_const(mScan, "$1", INT2NUM(SDL_SCANCODE_$1))')
    
define(`DEFINE_SCANCODE_NUMBER',`/$8* @return [Integer] scancode for number key "$1" (not on keypad) *$8/
    rb_define_const(mScan, "K$1", INT2NUM(SDL_SCANCODE_$1))')

define(`DEFINE_SCANCODE_ALPH',`/$8* @return [Integer] scancode for alphabet key "$1" *$8/
    rb_define_const(mScan, "$1", INT2NUM(SDL_SCANCODE_$1))')

define(`DEFINE_KEYCODE', `ifelse(`$#',`2',`$2
    ',`/$8* @return [Integer] keycode for "$1" key *$8/
    ')rb_define_const(mKey, "$1", INT2NUM(SDLK_$1))')
    
define(`DEFINE_KEYCODE_NUMBER',`/$8* @return [Integer] keycode for number key "$1" (not on keypad) *$8/
    rb_define_const(mKey, "K$1", INT2NUM(SDLK_$1))')
    
define(`DEFINE_KEYCODE_ALPH',`/$8* @return [Integer] keycode for alphabet key "$1" *$8/
    rb_define_const(mKey, "translit($1,`a-z',`A-Z')", INT2NUM(SDLK_$1))')

define(`DEFINE_KEYMOD',`rb_define_const(mMod, "$1", INT2NUM(KMOD_$1))')
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
    
    DEFINE_SCANCODE(UNKNOWN,/* @return [Integer] unused scancode */);
    DEFINE_SCANCODE_ALPH(A);
    DEFINE_SCANCODE_ALPH(B);
    DEFINE_SCANCODE_ALPH(C);
    DEFINE_SCANCODE_ALPH(D);
    DEFINE_SCANCODE_ALPH(E);
    DEFINE_SCANCODE_ALPH(F);
    DEFINE_SCANCODE_ALPH(G);
    DEFINE_SCANCODE_ALPH(H);
    DEFINE_SCANCODE_ALPH(I);
    DEFINE_SCANCODE_ALPH(J);
    DEFINE_SCANCODE_ALPH(K);
    DEFINE_SCANCODE_ALPH(L);
    DEFINE_SCANCODE_ALPH(M);
    DEFINE_SCANCODE_ALPH(N);
    DEFINE_SCANCODE_ALPH(O);
    DEFINE_SCANCODE_ALPH(P);
    DEFINE_SCANCODE_ALPH(Q);
    DEFINE_SCANCODE_ALPH(R);
    DEFINE_SCANCODE_ALPH(S);
    DEFINE_SCANCODE_ALPH(T);
    DEFINE_SCANCODE_ALPH(U);
    DEFINE_SCANCODE_ALPH(V);
    DEFINE_SCANCODE_ALPH(W);
    DEFINE_SCANCODE_ALPH(X);
    DEFINE_SCANCODE_ALPH(Y);
    DEFINE_SCANCODE_ALPH(Z);
    
    DEFINE_SCANCODE_NUMBER(1);
    DEFINE_SCANCODE_NUMBER(2);
    DEFINE_SCANCODE_NUMBER(3);
    DEFINE_SCANCODE_NUMBER(4);
    DEFINE_SCANCODE_NUMBER(5);
    DEFINE_SCANCODE_NUMBER(6);
    DEFINE_SCANCODE_NUMBER(7);
    DEFINE_SCANCODE_NUMBER(8);
    DEFINE_SCANCODE_NUMBER(9);
    DEFINE_SCANCODE_NUMBER(0);

    DEFINE_SCANCODE(RETURN);
    DEFINE_SCANCODE(ESCAPE);
    DEFINE_SCANCODE(BACKSPACE);
    DEFINE_SCANCODE(TAB);
    DEFINE_SCANCODE(SPACE);

    DEFINE_SCANCODE(MINUS);
    DEFINE_SCANCODE(EQUALS);
    DEFINE_SCANCODE(LEFTBRACKET);
    DEFINE_SCANCODE(RIGHTBRACKET);
    DEFINE_SCANCODE(BACKSLASH);

    DEFINE_SCANCODE(NONUSHASH);
    DEFINE_SCANCODE(SEMICOLON);
    DEFINE_SCANCODE(APOSTROPHE);
    DEFINE_SCANCODE(GRAVE);
    DEFINE_SCANCODE(COMMA);
    DEFINE_SCANCODE(PERIOD);
    DEFINE_SCANCODE(SLASH);

    DEFINE_SCANCODE(CAPSLOCK);

    DEFINE_SCANCODE(F1);
    DEFINE_SCANCODE(F2);
    DEFINE_SCANCODE(F3);
    DEFINE_SCANCODE(F4);
    DEFINE_SCANCODE(F5);
    DEFINE_SCANCODE(F6);
    DEFINE_SCANCODE(F7);
    DEFINE_SCANCODE(F8);
    DEFINE_SCANCODE(F9);
    DEFINE_SCANCODE(F10);
    DEFINE_SCANCODE(F11);
    DEFINE_SCANCODE(F12);

    DEFINE_SCANCODE(PRINTSCREEN);
    DEFINE_SCANCODE(SCROLLLOCK);
    DEFINE_SCANCODE(PAUSE);
    DEFINE_SCANCODE(INSERT);

    DEFINE_SCANCODE(HOME);
    DEFINE_SCANCODE(PAGEUP);
    DEFINE_SCANCODE(DELETE);
    DEFINE_SCANCODE(END);
    DEFINE_SCANCODE(PAGEDOWN);
    DEFINE_SCANCODE(RIGHT);
    DEFINE_SCANCODE(LEFT);
    DEFINE_SCANCODE(DOWN);
    DEFINE_SCANCODE(UP);

    DEFINE_SCANCODE(NUMLOCKCLEAR);

    DEFINE_SCANCODE(KP_DIVIDE);
    DEFINE_SCANCODE(KP_MULTIPLY);
    DEFINE_SCANCODE(KP_MINUS);
    DEFINE_SCANCODE(KP_PLUS);
    DEFINE_SCANCODE(KP_ENTER);
    DEFINE_SCANCODE(KP_1);
    DEFINE_SCANCODE(KP_2);
    DEFINE_SCANCODE(KP_3);
    DEFINE_SCANCODE(KP_4);
    DEFINE_SCANCODE(KP_5);
    DEFINE_SCANCODE(KP_6);
    DEFINE_SCANCODE(KP_7);
    DEFINE_SCANCODE(KP_8);
    DEFINE_SCANCODE(KP_9);
    DEFINE_SCANCODE(KP_0);
    DEFINE_SCANCODE(KP_PERIOD);

    DEFINE_SCANCODE(NONUSBACKSLASH);
    DEFINE_SCANCODE(APPLICATION);
    DEFINE_SCANCODE(POWER);
    DEFINE_SCANCODE(KP_EQUALS);
    DEFINE_SCANCODE(F13);
    DEFINE_SCANCODE(F14);
    DEFINE_SCANCODE(F15);
    DEFINE_SCANCODE(F16);
    DEFINE_SCANCODE(F17);
    DEFINE_SCANCODE(F18);
    DEFINE_SCANCODE(F19);
    DEFINE_SCANCODE(F20);
    DEFINE_SCANCODE(F21);
    DEFINE_SCANCODE(F22);
    DEFINE_SCANCODE(F23);
    DEFINE_SCANCODE(F24);
    DEFINE_SCANCODE(EXECUTE);
    DEFINE_SCANCODE(HELP);
    DEFINE_SCANCODE(MENU);
    DEFINE_SCANCODE(SELECT);
    DEFINE_SCANCODE(STOP);
    DEFINE_SCANCODE(AGAIN);
    DEFINE_SCANCODE(UNDO);
    DEFINE_SCANCODE(CUT);
    DEFINE_SCANCODE(COPY);
    DEFINE_SCANCODE(PASTE);
    DEFINE_SCANCODE(FIND);
    DEFINE_SCANCODE(MUTE);
    DEFINE_SCANCODE(VOLUMEUP);
    DEFINE_SCANCODE(VOLUMEDOWN);
    /* not sure whether there's a reason to enable these */
    /*     SDL_SCANCODE_LOCKINGCAPSLOCK = 130,  */
    /*     SDL_SCANCODE_LOCKINGNUMLOCK = 131, */
    /*     SDL_SCANCODE_LOCKINGSCROLLLOCK = 132, */
    DEFINE_SCANCODE(KP_COMMA);
    DEFINE_SCANCODE(KP_EQUALSAS400);

    DEFINE_SCANCODE(INTERNATIONAL1);
        
    DEFINE_SCANCODE(INTERNATIONAL2);
    DEFINE_SCANCODE(INTERNATIONAL3);
    DEFINE_SCANCODE(INTERNATIONAL4);
    DEFINE_SCANCODE(INTERNATIONAL5);
    DEFINE_SCANCODE(INTERNATIONAL6);
    DEFINE_SCANCODE(INTERNATIONAL7);
    DEFINE_SCANCODE(INTERNATIONAL8);
    DEFINE_SCANCODE(INTERNATIONAL9);
    DEFINE_SCANCODE(LANG1);
    DEFINE_SCANCODE(LANG2);
    DEFINE_SCANCODE(LANG3);
    DEFINE_SCANCODE(LANG4);
    DEFINE_SCANCODE(LANG5);
    DEFINE_SCANCODE(LANG6);
    DEFINE_SCANCODE(LANG7);
    DEFINE_SCANCODE(LANG8);
    DEFINE_SCANCODE(LANG9);

    DEFINE_SCANCODE(ALTERASE);
    DEFINE_SCANCODE(SYSREQ);
    DEFINE_SCANCODE(CANCEL);
    DEFINE_SCANCODE(CLEAR);
    DEFINE_SCANCODE(PRIOR);
    DEFINE_SCANCODE(RETURN2);
    DEFINE_SCANCODE(SEPARATOR);
    DEFINE_SCANCODE(OUT);
    DEFINE_SCANCODE(OPER);
    DEFINE_SCANCODE(CLEARAGAIN);
    DEFINE_SCANCODE(CRSEL);
    DEFINE_SCANCODE(EXSEL);

    DEFINE_SCANCODE(KP_00);
    DEFINE_SCANCODE(KP_000);
    DEFINE_SCANCODE(THOUSANDSSEPARATOR);
    DEFINE_SCANCODE(DECIMALSEPARATOR);
    DEFINE_SCANCODE(CURRENCYUNIT);
    DEFINE_SCANCODE(CURRENCYSUBUNIT);
    DEFINE_SCANCODE(KP_LEFTPAREN);
    DEFINE_SCANCODE(KP_RIGHTPAREN);
    DEFINE_SCANCODE(KP_LEFTBRACE);
    DEFINE_SCANCODE(KP_RIGHTBRACE);
    DEFINE_SCANCODE(KP_TAB);
    DEFINE_SCANCODE(KP_BACKSPACE);
    DEFINE_SCANCODE(KP_A);
    DEFINE_SCANCODE(KP_B);
    DEFINE_SCANCODE(KP_C);
    DEFINE_SCANCODE(KP_D);
    DEFINE_SCANCODE(KP_E);
    DEFINE_SCANCODE(KP_F);
    DEFINE_SCANCODE(KP_XOR);
    DEFINE_SCANCODE(KP_POWER);
    DEFINE_SCANCODE(KP_PERCENT);
    DEFINE_SCANCODE(KP_LESS);
    DEFINE_SCANCODE(KP_GREATER);
    DEFINE_SCANCODE(KP_AMPERSAND);
    DEFINE_SCANCODE(KP_DBLAMPERSAND);
    DEFINE_SCANCODE(KP_VERTICALBAR);
    DEFINE_SCANCODE(KP_DBLVERTICALBAR);
    DEFINE_SCANCODE(KP_COLON);
    DEFINE_SCANCODE(KP_HASH);
    DEFINE_SCANCODE(KP_SPACE);
    DEFINE_SCANCODE(KP_AT);
    DEFINE_SCANCODE(KP_EXCLAM);
    DEFINE_SCANCODE(KP_MEMSTORE);
    DEFINE_SCANCODE(KP_MEMRECALL);
    DEFINE_SCANCODE(KP_MEMCLEAR);
    DEFINE_SCANCODE(KP_MEMADD);
    DEFINE_SCANCODE(KP_MEMSUBTRACT);
    DEFINE_SCANCODE(KP_MEMMULTIPLY);
    DEFINE_SCANCODE(KP_MEMDIVIDE);
    DEFINE_SCANCODE(KP_PLUSMINUS);
    DEFINE_SCANCODE(KP_CLEAR);
    DEFINE_SCANCODE(KP_CLEARENTRY);
    DEFINE_SCANCODE(KP_BINARY);
    DEFINE_SCANCODE(KP_OCTAL);
    DEFINE_SCANCODE(KP_DECIMAL);
    DEFINE_SCANCODE(KP_HEXADECIMAL);

    DEFINE_SCANCODE(LCTRL);
    DEFINE_SCANCODE(LSHIFT);
    DEFINE_SCANCODE(LALT);
    DEFINE_SCANCODE(LGUI);
    DEFINE_SCANCODE(RCTRL);
    DEFINE_SCANCODE(RSHIFT);
    DEFINE_SCANCODE(RALT);
    DEFINE_SCANCODE(RGUI);

    DEFINE_SCANCODE(MODE);

    DEFINE_SCANCODE(AUDIONEXT);
    DEFINE_SCANCODE(AUDIOPREV);
    DEFINE_SCANCODE(AUDIOSTOP);
    DEFINE_SCANCODE(AUDIOPLAY);
    DEFINE_SCANCODE(AUDIOMUTE);
    DEFINE_SCANCODE(MEDIASELECT);
    DEFINE_SCANCODE(WWW);
    DEFINE_SCANCODE(MAIL);
    DEFINE_SCANCODE(CALCULATOR);
    DEFINE_SCANCODE(COMPUTER);
    DEFINE_SCANCODE(AC_SEARCH);
    DEFINE_SCANCODE(AC_HOME);
    DEFINE_SCANCODE(AC_BACK);
    DEFINE_SCANCODE(AC_FORWARD);
    DEFINE_SCANCODE(AC_STOP);
    DEFINE_SCANCODE(AC_REFRESH);
    DEFINE_SCANCODE(AC_BOOKMARKS);

    DEFINE_SCANCODE(BRIGHTNESSDOWN);
    DEFINE_SCANCODE(BRIGHTNESSUP);
    DEFINE_SCANCODE(DISPLAYSWITCH);

    DEFINE_SCANCODE(KBDILLUMTOGGLE);
    DEFINE_SCANCODE(KBDILLUMDOWN);
    DEFINE_SCANCODE(KBDILLUMUP);
    DEFINE_SCANCODE(EJECT);
    DEFINE_SCANCODE(SLEEP);

    DEFINE_SCANCODE(APP1);
    DEFINE_SCANCODE(APP2);

    DEFINE_KEYCODE(UNKNOWN,/* @return [Integer] unused keycode */);
    DEFINE_KEYCODE(RETURN);
    DEFINE_KEYCODE(ESCAPE);
    DEFINE_KEYCODE(BACKSPACE);
    DEFINE_KEYCODE(TAB);
    DEFINE_KEYCODE(SPACE);
    DEFINE_KEYCODE(EXCLAIM);
    DEFINE_KEYCODE(QUOTEDBL);
    DEFINE_KEYCODE(HASH);
    DEFINE_KEYCODE(PERCENT);
    DEFINE_KEYCODE(DOLLAR);
    DEFINE_KEYCODE(AMPERSAND);
    DEFINE_KEYCODE(QUOTE);
    DEFINE_KEYCODE(LEFTPAREN);
    DEFINE_KEYCODE(RIGHTPAREN);
    DEFINE_KEYCODE(ASTERISK);
    DEFINE_KEYCODE(PLUS);
    DEFINE_KEYCODE(COMMA);
    DEFINE_KEYCODE(MINUS);
    DEFINE_KEYCODE(PERIOD);
    DEFINE_KEYCODE(SLASH);
    DEFINE_KEYCODE_NUMBER(0);
    DEFINE_KEYCODE_NUMBER(1);
    DEFINE_KEYCODE_NUMBER(2);
    DEFINE_KEYCODE_NUMBER(3);
    DEFINE_KEYCODE_NUMBER(4);
    DEFINE_KEYCODE_NUMBER(5);
    DEFINE_KEYCODE_NUMBER(6);
    DEFINE_KEYCODE_NUMBER(7);
    DEFINE_KEYCODE_NUMBER(8);
    DEFINE_KEYCODE_NUMBER(9);
    DEFINE_KEYCODE(COLON);
    DEFINE_KEYCODE(SEMICOLON);
    DEFINE_KEYCODE(LESS);
    DEFINE_KEYCODE(EQUALS);
    DEFINE_KEYCODE(GREATER);
    DEFINE_KEYCODE(QUESTION);
    DEFINE_KEYCODE(AT);
    /*
       Skip uppercase letters
     */
    DEFINE_KEYCODE(LEFTBRACKET);
    DEFINE_KEYCODE(BACKSLASH);
    DEFINE_KEYCODE(RIGHTBRACKET);
    DEFINE_KEYCODE(CARET);
    DEFINE_KEYCODE(UNDERSCORE);
    DEFINE_KEYCODE(BACKQUOTE);
    
    DEFINE_KEYCODE_ALPH(a);
    DEFINE_KEYCODE_ALPH(b);
    DEFINE_KEYCODE_ALPH(c);
    DEFINE_KEYCODE_ALPH(d);
    DEFINE_KEYCODE_ALPH(e);
    DEFINE_KEYCODE_ALPH(f);
    DEFINE_KEYCODE_ALPH(g);
    DEFINE_KEYCODE_ALPH(h);
    DEFINE_KEYCODE_ALPH(i);
    DEFINE_KEYCODE_ALPH(j);
    DEFINE_KEYCODE_ALPH(k);
    DEFINE_KEYCODE_ALPH(l);
    DEFINE_KEYCODE_ALPH(m);
    DEFINE_KEYCODE_ALPH(n);
    DEFINE_KEYCODE_ALPH(o);
    DEFINE_KEYCODE_ALPH(p);
    DEFINE_KEYCODE_ALPH(q);
    DEFINE_KEYCODE_ALPH(r);
    DEFINE_KEYCODE_ALPH(s);
    DEFINE_KEYCODE_ALPH(t);
    DEFINE_KEYCODE_ALPH(u);
    DEFINE_KEYCODE_ALPH(v);
    DEFINE_KEYCODE_ALPH(w);
    DEFINE_KEYCODE_ALPH(x);
    DEFINE_KEYCODE_ALPH(y);
    DEFINE_KEYCODE_ALPH(z);

    DEFINE_KEYCODE(CAPSLOCK);

    DEFINE_KEYCODE(F1);
    DEFINE_KEYCODE(F2);
    DEFINE_KEYCODE(F3);
    DEFINE_KEYCODE(F4);
    DEFINE_KEYCODE(F5);
    DEFINE_KEYCODE(F6);
    DEFINE_KEYCODE(F7);
    DEFINE_KEYCODE(F8);
    DEFINE_KEYCODE(F9);
    DEFINE_KEYCODE(F10);
    DEFINE_KEYCODE(F11);
    DEFINE_KEYCODE(F12);

    DEFINE_KEYCODE(PRINTSCREEN);
    DEFINE_KEYCODE(SCROLLLOCK);
    DEFINE_KEYCODE(PAUSE);
    DEFINE_KEYCODE(INSERT);
    DEFINE_KEYCODE(HOME);
    DEFINE_KEYCODE(PAGEUP);
    DEFINE_KEYCODE(DELETE);
    DEFINE_KEYCODE(END);
    DEFINE_KEYCODE(PAGEDOWN);
    DEFINE_KEYCODE(RIGHT);
    DEFINE_KEYCODE(LEFT);
    DEFINE_KEYCODE(DOWN);
    DEFINE_KEYCODE(UP);

    DEFINE_KEYCODE(NUMLOCKCLEAR);
    DEFINE_KEYCODE(KP_DIVIDE);
    DEFINE_KEYCODE(KP_MULTIPLY);
    DEFINE_KEYCODE(KP_MINUS);
    DEFINE_KEYCODE(KP_PLUS);
    DEFINE_KEYCODE(KP_ENTER);
    DEFINE_KEYCODE(KP_1);
    DEFINE_KEYCODE(KP_2);
    DEFINE_KEYCODE(KP_3);
    DEFINE_KEYCODE(KP_4);
    DEFINE_KEYCODE(KP_5);
    DEFINE_KEYCODE(KP_6);
    DEFINE_KEYCODE(KP_7);
    DEFINE_KEYCODE(KP_8);
    DEFINE_KEYCODE(KP_9);
    DEFINE_KEYCODE(KP_0);
    DEFINE_KEYCODE(KP_PERIOD);

    DEFINE_KEYCODE(APPLICATION);
    DEFINE_KEYCODE(POWER);
    DEFINE_KEYCODE(KP_EQUALS);
    DEFINE_KEYCODE(F13);
    DEFINE_KEYCODE(F14);
    DEFINE_KEYCODE(F15);
    DEFINE_KEYCODE(F16);
    DEFINE_KEYCODE(F17);
    DEFINE_KEYCODE(F18);
    DEFINE_KEYCODE(F19);
    DEFINE_KEYCODE(F20);
    DEFINE_KEYCODE(F21);
    DEFINE_KEYCODE(F22);
    DEFINE_KEYCODE(F23);
    DEFINE_KEYCODE(F24);
    DEFINE_KEYCODE(EXECUTE);
    DEFINE_KEYCODE(HELP);
    DEFINE_KEYCODE(MENU);
    DEFINE_KEYCODE(SELECT);
    DEFINE_KEYCODE(STOP);
    DEFINE_KEYCODE(AGAIN);
    DEFINE_KEYCODE(UNDO);
    DEFINE_KEYCODE(CUT);
    DEFINE_KEYCODE(COPY);
    DEFINE_KEYCODE(PASTE);
    DEFINE_KEYCODE(FIND);
    DEFINE_KEYCODE(MUTE);
    DEFINE_KEYCODE(VOLUMEUP);
    DEFINE_KEYCODE(VOLUMEDOWN);
    DEFINE_KEYCODE(KP_COMMA);
    DEFINE_KEYCODE(KP_EQUALSAS400);
    
    DEFINE_KEYCODE(ALTERASE);
    DEFINE_KEYCODE(SYSREQ);
    DEFINE_KEYCODE(CANCEL);
    DEFINE_KEYCODE(CLEAR);
    DEFINE_KEYCODE(PRIOR);
    DEFINE_KEYCODE(RETURN2);
    DEFINE_KEYCODE(SEPARATOR);
    DEFINE_KEYCODE(OUT);
    DEFINE_KEYCODE(OPER);
    DEFINE_KEYCODE(CLEARAGAIN);
    DEFINE_KEYCODE(CRSEL);
    DEFINE_KEYCODE(EXSEL);

    DEFINE_KEYCODE(KP_00);
    DEFINE_KEYCODE(KP_000);
    DEFINE_KEYCODE(THOUSANDSSEPARATOR);

    DEFINE_KEYCODE(DECIMALSEPARATOR);

    DEFINE_KEYCODE(CURRENCYUNIT);
    DEFINE_KEYCODE(CURRENCYSUBUNIT);
    DEFINE_KEYCODE(KP_LEFTPAREN);
    DEFINE_KEYCODE(KP_RIGHTPAREN);
    DEFINE_KEYCODE(KP_LEFTBRACE);
    DEFINE_KEYCODE(KP_RIGHTBRACE);
    DEFINE_KEYCODE(KP_TAB);
    DEFINE_KEYCODE(KP_BACKSPACE);
    DEFINE_KEYCODE(KP_A);
    DEFINE_KEYCODE(KP_B);
    DEFINE_KEYCODE(KP_C);
    DEFINE_KEYCODE(KP_D);
    DEFINE_KEYCODE(KP_E);
    DEFINE_KEYCODE(KP_F);
    DEFINE_KEYCODE(KP_XOR);
    DEFINE_KEYCODE(KP_POWER);
    DEFINE_KEYCODE(KP_PERCENT);
    DEFINE_KEYCODE(KP_LESS);
    DEFINE_KEYCODE(KP_GREATER);
    DEFINE_KEYCODE(KP_AMPERSAND);
    DEFINE_KEYCODE(KP_DBLAMPERSAND);
    DEFINE_KEYCODE(KP_VERTICALBAR);
    DEFINE_KEYCODE(KP_DBLVERTICALBAR);
    DEFINE_KEYCODE(KP_COLON);
    DEFINE_KEYCODE(KP_HASH);
    DEFINE_KEYCODE(KP_SPACE);
    DEFINE_KEYCODE(KP_AT);
    DEFINE_KEYCODE(KP_EXCLAM);
    DEFINE_KEYCODE(KP_MEMSTORE);
    DEFINE_KEYCODE(KP_MEMRECALL);
    DEFINE_KEYCODE(KP_MEMCLEAR);
    DEFINE_KEYCODE(KP_MEMADD);
    DEFINE_KEYCODE(KP_MEMSUBTRACT);
    DEFINE_KEYCODE(KP_MEMMULTIPLY);
    DEFINE_KEYCODE(KP_MEMDIVIDE);
    DEFINE_KEYCODE(KP_PLUSMINUS);
    DEFINE_KEYCODE(KP_CLEAR);
    DEFINE_KEYCODE(KP_CLEARENTRY);
    DEFINE_KEYCODE(KP_BINARY);
    DEFINE_KEYCODE(KP_OCTAL);
    DEFINE_KEYCODE(KP_DECIMAL);
    DEFINE_KEYCODE(KP_HEXADECIMAL);

    DEFINE_KEYCODE(LCTRL);
    DEFINE_KEYCODE(LSHIFT);
    DEFINE_KEYCODE(LALT);
    DEFINE_KEYCODE(LGUI);
    DEFINE_KEYCODE(RCTRL);
    DEFINE_KEYCODE(RSHIFT);
    DEFINE_KEYCODE(RALT);
    DEFINE_KEYCODE(RGUI);

    DEFINE_KEYCODE(MODE);

    DEFINE_KEYCODE(AUDIONEXT);
    DEFINE_KEYCODE(AUDIOPREV);
    DEFINE_KEYCODE(AUDIOSTOP);
    DEFINE_KEYCODE(AUDIOPLAY);
    DEFINE_KEYCODE(AUDIOMUTE);
    DEFINE_KEYCODE(MEDIASELECT);
    DEFINE_KEYCODE(WWW);
    DEFINE_KEYCODE(MAIL);
    DEFINE_KEYCODE(CALCULATOR);
    DEFINE_KEYCODE(COMPUTER);
    DEFINE_KEYCODE(AC_SEARCH);
    DEFINE_KEYCODE(AC_HOME);
    DEFINE_KEYCODE(AC_BACK);
    DEFINE_KEYCODE(AC_FORWARD);
    DEFINE_KEYCODE(AC_STOP);
    DEFINE_KEYCODE(AC_REFRESH);
    DEFINE_KEYCODE(AC_BOOKMARKS);

    DEFINE_KEYCODE(BRIGHTNESSDOWN);

    DEFINE_KEYCODE(BRIGHTNESSUP);
    DEFINE_KEYCODE(DISPLAYSWITCH);
    DEFINE_KEYCODE(KBDILLUMTOGGLE);

    DEFINE_KEYCODE(KBDILLUMDOWN);
    DEFINE_KEYCODE(KBDILLUMUP);
    DEFINE_KEYCODE(EJECT);
    DEFINE_KEYCODE(SLEEP);

    /* 0 (no modifier is applicable) */
    DEFINE_KEYMOD(NONE);
    /* the modifier key bit mask for the left shift key */
    DEFINE_KEYMOD(LSHIFT);
    /* the modifier key bit mask for the right shift key */
    DEFINE_KEYMOD(RSHIFT);
    /* the modifier key bit mask for the left control key */
    DEFINE_KEYMOD(LCTRL);
    /* the modifier key bit mask for the right control key */
    DEFINE_KEYMOD(RCTRL);
    /* the modifier key bit mask for the left alt key */
    DEFINE_KEYMOD(LALT);
    /* the modifier key bit mask for the right alt key */
    DEFINE_KEYMOD(RALT);
    /* the modifier key bit mask for the left GUI key (often the window key) */
    DEFINE_KEYMOD(LGUI);
    /* the modifier key bit mask for the right GUI key (often the window key) */
    DEFINE_KEYMOD(RGUI);
    /* the modifier key bit mask for the numlock key */
    DEFINE_KEYMOD(NUM);
    /* the modifier key bit mask for the capslock key */
    DEFINE_KEYMOD(CAPS);
    /* the modifier key bit mask for the mode key (AltGr) */
    DEFINE_KEYMOD(MODE);
    /* the modifier key bit mask for the left and right control key */
    DEFINE_KEYMOD(CTRL);
    /* the modifier key bit mask for the left and right shift key */
    DEFINE_KEYMOD(SHIFT);
    /* the modifier key bit mask for the left and right alt key */
    DEFINE_KEYMOD(ALT);
    /* the modifier key bit mask for the left and right GUI key */
    DEFINE_KEYMOD(GUI);
    /* reserved for future use */
    DEFINE_KEYMOD(RESERVED);
}
