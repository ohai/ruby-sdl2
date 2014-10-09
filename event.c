#include "rubysdl2_internal.h"
#include <SDL_events.h>

static VALUE cEvent;
static VALUE cEvQuit;
static VALUE cEvWindow;
static VALUE cEvSysWM;
static VALUE cEvKeyboard;
static VALUE cEvKeyDown;
static VALUE cEvKeyUp;
static VALUE cEvTextEditing;
static VALUE cEvTextInput;
/* static VALUE cEvMouseButton; */
/* static VALUE cEvMouseButtonDown; */
/* static VALUE cEvMouseButtonUp; */
/* static VALUE cEvMouseMotion; */
/* static VALUE cEvMouseWheel; */
/* static VALUE cEvJoyAxisMotion; */
/* static VALUE cEvJoyBallMotion; */
/* static VALUE cEvJoyButton; */
/* static VALUE cEvJoyButtonDown; */
/* static VALUE cEvJoyButtonUp; */
/* static VALUE cEvJoyHatMotion; */
/* static VALUE cEvJoyDevice; */
/* static VALUE cEvJoyDeviceAdded; */
/* static VALUE cEvJoyDeviceRemoved; */
/* static VALUE cEvControllerAxisMotion; */
/* static VALUE cEvControllerButton; */
/* static VALUE cEvControllerButtonDown; */
/* static VALUE cEvControllerButtonUp; */
/* static VALUE cEvControllerDevice; */
/* static VALUE cEvControllerDeviceAdded; */
/* static VALUE cEvControllerDeviceRemoved; */
/* static VALUE cEvControllerDeviceRemapped; */
/* static VALUE cEvUser; */
/* static VALUE cEvDrop; */


static VALUE event_type_to_class[SDL_LASTEVENT];

static VALUE Event_new(SDL_Event* ev)
{
    SDL_Event* e = ALLOC(SDL_Event);
    *e = *ev;
    return Data_Wrap_Struct(event_type_to_class[ev->type], 0, free, e);
}

static VALUE Event_s_poll(VALUE self)
{
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
        return Event_new(&ev);
    } else {
        return Qnil;
    }
}


static void set_string(char* field, VALUE str, int maxlength)
{
    StringValueCStr(str);
    if (RSTRING_LEN(str) > maxlength)
        rb_raise(rb_eArgError, "string length must be less than %d", maxlength);
    strcpy(field, RSTRING_PTR(str));
}

#define EVENT_READER(classname, name, field, c2ruby)            \
    static VALUE classname##_##name(VALUE self)                 \
    {                                                           \
        SDL_Event* ev;                                          \
        Data_Get_Struct(self, SDL_Event, ev);                   \
        return c2ruby(ev->field);                               \
    }                                                           \

#define EVENT_WRITER(classname, name, field, ruby2c)            \
    static VALUE classname##_set_##name(VALUE self, VALUE val)  \
    {                                                           \
        SDL_Event* ev;                                          \
        Data_Get_Struct(self, SDL_Event, ev);                   \
        ev->field = ruby2c(val);                                \
        return Qnil;                                            \
    }

#define EVENT_ACCESSOR(classname, name, field, ruby2c, c2ruby)  \
    EVENT_READER(classname, name, field, c2ruby)                \
    EVENT_WRITER(classname, name, field, ruby2c)

#define EVENT_ACCESSOR_UINT(classname, name, field) \
    EVENT_ACCESSOR(classname, name, field, NUM2UINT, UINT2NUM)

#define EVENT_ACCESSOR_INT(classname, name, field) \
    EVENT_ACCESSOR(classname, name, field, NUM2INT, INT2NUM)

EVENT_ACCESSOR_UINT(Event, timestamp, common.timestamp);

EVENT_ACCESSOR_UINT(Window, window_id, window.windowID);
EVENT_ACCESSOR_UINT(Window, event, window.event);
EVENT_ACCESSOR_INT(Window, data1, window.data1);
EVENT_ACCESSOR_INT(Window, data2, window.data2);

EVENT_ACCESSOR_UINT(Keyboard, state, key.state);
EVENT_ACCESSOR_UINT(Keyboard, scancode, key.keysym.scancode);
EVENT_ACCESSOR_UINT(Keyboard, sym, key.keysym.sym);
EVENT_ACCESSOR_UINT(Keyboard, mod, key.keysym.mod);

EVENT_ACCESSOR_UINT(TextEditing, window_id, edit.windowID);
EVENT_ACCESSOR_INT(TextEditing, start, edit.start);
EVENT_ACCESSOR_INT(TextEditing, length, edit.length);
EVENT_READER(TextEditing, text, edit.text, utf8str_new_cstr);
static VALUE TextEditing_set_text(VALUE self, VALUE str)
{
    SDL_Event* ev;
    Data_Get_Struct(self, SDL_Event, ev);
    set_string(ev->edit.text, str, 30);
    return str;
}

EVENT_ACCESSOR_UINT(TextInput, window_id, text.windowID);
EVENT_READER(TextInput, text, text.text, utf8str_new_cstr);
static VALUE TextInput_set_text(VALUE self, VALUE str)
{
    SDL_Event* ev;
    Data_Get_Struct(self, SDL_Event, ev);
    set_string(ev->text.text, str, 30);
    return str;
}

static void init_event_type_to_class(void)
{
    int i;
    for (i=0; i<SDL_LASTEVENT; ++i)
        event_type_to_class[i] = cEvent;

    event_type_to_class[SDL_QUIT] = cEvQuit;
    event_type_to_class[SDL_WINDOWEVENT] = cEvWindow;
    event_type_to_class[SDL_KEYDOWN] = cEvKeyDown;
    event_type_to_class[SDL_KEYUP] = cEvKeyUp;
    event_type_to_class[SDL_TEXTEDITING] = cEvTextEditing;
    event_type_to_class[SDL_TEXTINPUT] = cEvTextInput;
}

#define DEFINE_EVENT_READER(classname, classvar, name) \
    rb_define_method(classvar, #name, classname##_##name, 0)
#define DEFINE_EVENT_WRITER(classname, classvar, name) \
    rb_define_method(classvar, #name "=", classname##_set_##name, 1)

#define DEFINE_EVENT_ACCESSOR(classname, classvar, name)                \
    do {                                                                \
        DEFINE_EVENT_READER(classname, classvar, name);                 \
        DEFINE_EVENT_WRITER(classname, classvar, name);                 \
    } while(0)

void rubysdl2_init_event(void)
{
    cEvent = rb_define_class_under(mSDL2, "Event", rb_cObject);
    rb_define_singleton_method(cEvent, "poll", Event_s_poll, 0);

    cEvQuit = rb_define_class_under(cEvent, "Quit", cEvent);
    cEvWindow = rb_define_class_under(cEvent, "Window", cEvent);
    cEvSysWM = rb_define_class_under(cEvent, "SysWM", cEvent);
    cEvKeyboard = rb_define_class_under(cEvent, "Keyboard", cEvent);
    cEvKeyUp = rb_define_class_under(cEvent, "KeyUp", cEvKeyboard);
    cEvKeyDown = rb_define_class_under(cEvent, "KeyDown", cEvKeyboard);
    cEvTextEditing = rb_define_class_under(cEvent, "TextEditing", cEvent);
    cEvTextInput = rb_define_class_under(cEvent, "TextInput", cEvent);
    
    DEFINE_EVENT_ACCESSOR(Event, cEvent, timestamp);

    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, window_id);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, event);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, data1);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, data2);
    
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, state);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, scancode);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, sym);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, mod);

    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, window_id);
    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, text);
    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, start);
    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, length);

    DEFINE_EVENT_ACCESSOR(TextInput, cEvTextInput, window_id);
    DEFINE_EVENT_ACCESSOR(TextInput, cEvTextInput, text);
    
    init_event_type_to_class();
}
