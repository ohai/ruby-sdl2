#include "rubysdl2_internal.h"
#include <SDL_events.h>
#include <SDL_version.h>

static VALUE cEvent;
static VALUE cEvQuit;
static VALUE cEvWindow;
static VALUE cEvSysWM;
static VALUE cEvKeyboard;
static VALUE cEvKeyDown;
static VALUE cEvKeyUp;
static VALUE cEvTextEditing;
static VALUE cEvTextInput;
static VALUE cEvMouseButton;
static VALUE cEvMouseButtonDown;
static VALUE cEvMouseButtonUp;
static VALUE cEvMouseMotion;
static VALUE cEvMouseWheel;
static VALUE cEvJoyAxisMotion;
static VALUE cEvJoyBallMotion;
static VALUE cEvJoyButton;
static VALUE cEvJoyButtonDown;
static VALUE cEvJoyButtonUp;
static VALUE cEvJoyHatMotion;
static VALUE cEvJoyDevice;
static VALUE cEvJoyDeviceAdded;
static VALUE cEvJoyDeviceRemoved;
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


static const char* INT2BOOLSTR(int bool)
{
    return bool ? "true" : "false";
}

static VALUE event_type_to_class[SDL_LASTEVENT];

static VALUE Event_new(SDL_Event* ev)
{
    SDL_Event* e = ALLOC(SDL_Event);
    *e = *ev;
    return Data_Wrap_Struct(event_type_to_class[ev->type], 0, free, e);
}

static VALUE Event_s_allocate(VALUE klass)
{
    SDL_Event* e;
    VALUE event_type = rb_iv_get(klass, "event_type");
    if (event_type == Qnil)
        rb_raise(rb_eArgError, "Cannot allocate %s", rb_class2name(klass));
    
    e = ALLOC(SDL_Event);
    memset(e, 0, sizeof(SDL_Event));
    e->common.type = NUM2INT(event_type);
    return Data_Wrap_Struct(klass, 0, free, e);
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

static VALUE Event_s_enabled_p(VALUE self)
{
    VALUE event_type = rb_iv_get(self, "event_type");
    if (event_type == Qnil) {
        rb_warn("You cannot enable %s directly", rb_class2name(self));
        return Qfalse;
    }
    return INT2BOOL(SDL_EventState(NUM2INT(event_type), SDL_QUERY) == SDL_ENABLE);
}

static void set_string(char* field, VALUE str, int maxlength)
{
    StringValueCStr(str);
    if (RSTRING_LEN(str) > maxlength)
        rb_raise(rb_eArgError, "string length must be less than %d", maxlength);
    strcpy(field, RSTRING_PTR(str));
}

#define EVENT_READER(classname, name, field, c2ruby)            \
    static VALUE Ev##classname##_##name(VALUE self)             \
    {                                                           \
        SDL_Event* ev;                                          \
        Data_Get_Struct(self, SDL_Event, ev);                   \
        return c2ruby(ev->field);                               \
    }                                                           \

#define EVENT_WRITER(classname, name, field, ruby2c)                    \
    static VALUE Ev##classname##_set_##name(VALUE self, VALUE val)      \
    {                                                                   \
        SDL_Event* ev;                                                  \
        Data_Get_Struct(self, SDL_Event, ev);                           \
        ev->field = ruby2c(val);                                        \
        return Qnil;                                                    \
    }

#define EVENT_ACCESSOR(classname, name, field, ruby2c, c2ruby)  \
    EVENT_READER(classname, name, field, c2ruby)                \
    EVENT_WRITER(classname, name, field, ruby2c)

#define EVENT_ACCESSOR_UINT(classname, name, field) \
    EVENT_ACCESSOR(classname, name, field, NUM2UINT, UINT2NUM)

#define EVENT_ACCESSOR_UINT8(classname, name, field) \
    EVENT_ACCESSOR(classname, name, field, NUM2UCHAR, UCHAR2NUM)

#define EVENT_ACCESSOR_INT(classname, name, field) \
    EVENT_ACCESSOR(classname, name, field, NUM2INT, INT2NUM)

#define EVENT_ACCESSOR_BOOL(classname, name, field)             \
    EVENT_ACCESSOR(classname, name, field, RTEST, INT2BOOL)


EVENT_READER(Event, type, common.type, INT2NUM);
EVENT_ACCESSOR_UINT(Event, timestamp, common.timestamp);
static VALUE Event_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp);
}


EVENT_ACCESSOR_UINT(Window, window_id, window.windowID);
EVENT_ACCESSOR_UINT(Window, event, window.event);
EVENT_ACCESSOR_INT(Window, data1, window.data1);
EVENT_ACCESSOR_INT(Window, data2, window.data2);
static VALUE EvWindow_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u window_id=%u event=%u data1=%d data2=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->window.windowID, ev->window.event,
                      ev->window.data1, ev->window.data2);
}

EVENT_ACCESSOR_UINT(Keyboard, window_id, key.windowID);
EVENT_ACCESSOR_UINT(Keyboard, state, key.state);
EVENT_ACCESSOR_UINT8(Keyboard, repeat, key.repeat);
EVENT_ACCESSOR_UINT8(Keyboard, scancode, key.keysym.scancode);
EVENT_ACCESSOR_UINT(Keyboard, sym, key.keysym.sym);
EVENT_ACCESSOR_UINT(Keyboard, mod, key.keysym.mod);
static VALUE EvKeyboard_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " window_id=%u state=%u repeat=%u"
                      " scancode=%u sym=%u mod=%u>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->key.windowID, ev->key.state, ev->key.repeat,
                      ev->key.keysym.scancode, ev->key.keysym.sym, ev->key.keysym.mod);
}


EVENT_ACCESSOR_UINT(TextEditing, window_id, edit.windowID);
EVENT_ACCESSOR_INT(TextEditing, start, edit.start);
EVENT_ACCESSOR_INT(TextEditing, length, edit.length);
EVENT_READER(TextEditing, text, edit.text, utf8str_new_cstr);
static VALUE EvTextEditing_set_text(VALUE self, VALUE str)
{
    SDL_Event* ev;
    Data_Get_Struct(self, SDL_Event, ev);
    set_string(ev->edit.text, str, 30);
    return str;
}

static VALUE EvTextEditing_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " window_id=%u text=%s start=%d length=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->edit.windowID, ev->edit.text, ev->edit.start, ev->edit.length);
}


EVENT_ACCESSOR_UINT(TextInput, window_id, text.windowID);
EVENT_READER(TextInput, text, text.text, utf8str_new_cstr);
static VALUE EvTextInput_set_text(VALUE self, VALUE str)
{
    SDL_Event* ev;
    Data_Get_Struct(self, SDL_Event, ev);
    set_string(ev->text.text, str, 30);
    return str;
}

static VALUE EvTextInput_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u window_id=%u text=%s>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->text.windowID, ev->text.text);
}


EVENT_ACCESSOR_UINT(MouseButton, window_id, button.windowID);
EVENT_ACCESSOR_UINT(MouseButton, which, button.which);
EVENT_ACCESSOR_UINT8(MouseButton, button, button.button);
EVENT_ACCESSOR_BOOL(MouseButton, pressed, button.state);
#if SDL_VERSION_ATLEAST(2,0,2)
EVENT_ACCESSOR_UINT8(MouseButton, clicks, button.clicks);
#endif
EVENT_ACCESSOR_INT(MouseButton, x, button.x);
EVENT_ACCESSOR_INT(MouseButton, y, button.y);
static VALUE EvMouseButton_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " window_id=%u which=%u button=%hhu pressed=%s"
#if SDL_VERSION_ATLEAST(2,0,2)
                      " clicks=%hhu"
#endif
                      " x=%d y=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->button.windowID, ev->button.which,
                      ev->button.button, INT2BOOLSTR(ev->button.state),
#if SDL_VERSION_ATLEAST(2,0,2)
                      ev->button.clicks,
#endif
                      ev->button.x, ev->button.y);
}


EVENT_ACCESSOR_UINT(MouseMotion, window_id, motion.windowID);
EVENT_ACCESSOR_UINT(MouseMotion, which, motion.which);
EVENT_ACCESSOR_UINT(MouseMotion, state, motion.state);
EVENT_ACCESSOR_INT(MouseMotion, x, motion.x);
EVENT_ACCESSOR_INT(MouseMotion, y, motion.y);
EVENT_ACCESSOR_INT(MouseMotion, xrel, motion.xrel);
EVENT_ACCESSOR_INT(MouseMotion, yrel, motion.yrel);
static VALUE EvMouseMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " window_id=%u which=%u state=%u"
                      " x=%d y=%d xrel=%d yrel=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->motion.windowID, ev->motion.which, ev->motion.state,
                      ev->motion.x, ev->motion.y, ev->motion.xrel, ev->motion.yrel);
}


EVENT_ACCESSOR_UINT(MouseWheel, window_id, wheel.windowID);
EVENT_ACCESSOR_UINT(MouseWheel, which, wheel.which);
EVENT_ACCESSOR_INT(MouseWheel, x, wheel.x);
EVENT_ACCESSOR_INT(MouseWheel, y, wheel.y);
static VALUE EvMouseWheel_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " window_id=%u which=%u x=%d y=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->wheel.windowID, ev->wheel.which, ev->wheel.x, ev->wheel.y);
}


EVENT_ACCESSOR_INT(JoyButton, which, jbutton.which);
EVENT_ACCESSOR_UINT8(JoyButton, button, jbutton.button);
EVENT_ACCESSOR_BOOL(JoyButton, pressed, jbutton.state)
static VALUE EvJoyButton_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d button=%u pressed=%s>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jbutton.which, ev->jbutton.button,
                      INT2BOOLSTR(ev->jbutton.state));
}


EVENT_ACCESSOR_INT(JoyAxisMotion, which, jaxis.which);
EVENT_ACCESSOR_UINT8(JoyAxisMotion, axis, jaxis.axis);
EVENT_ACCESSOR_INT(JoyAxisMotion, value, jaxis.value);
static VALUE EvJoyAxisMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d axis=%u value=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jaxis.which, ev->jaxis.axis, ev->jaxis.value);
}


EVENT_ACCESSOR_INT(JoyBallMotion, which, jball.which);
EVENT_ACCESSOR_UINT8(JoyBallMotion, ball, jball.ball);
EVENT_ACCESSOR_INT(JoyBallMotion, xrel, jball.xrel);
EVENT_ACCESSOR_INT(JoyBallMotion, yrel, jball.yrel);
static VALUE EvJoyBallMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d ball=%u xrel=%d yrel=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jball.which, ev->jball.ball, ev->jball.xrel, ev->jball.yrel);
}


EVENT_ACCESSOR_INT(JoyHatMotion, which, jhat.which);
EVENT_ACCESSOR_UINT8(JoyHatMotion, hat, jhat.hat);
EVENT_ACCESSOR_UINT8(JoyHatMotion, value, jhat.value);
static VALUE EvJoyHatMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u which=%d hat=%u value=%u>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jhat.which, ev->jhat.hat, ev->jhat.value);
}

EVENT_ACCESSOR_INT(JoyDevice, which, jdevice.which);
static VALUE EvJoyDevice_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u which=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jdevice.which);
}


static void connect_event_class(SDL_EventType type, VALUE klass)
{
    event_type_to_class[type] = klass;
    rb_iv_set(klass, "event_type", INT2NUM(type));
}

static void init_event_type_to_class(void)
{
    int i;
    for (i=0; i<SDL_LASTEVENT; ++i)
        event_type_to_class[i] = cEvent;
    
    connect_event_class(SDL_QUIT, cEvQuit);
    connect_event_class(SDL_WINDOWEVENT, cEvWindow);
    connect_event_class(SDL_KEYDOWN, cEvKeyDown);
    connect_event_class(SDL_KEYUP, cEvKeyUp);
    connect_event_class(SDL_TEXTEDITING, cEvTextEditing);
    connect_event_class(SDL_TEXTINPUT, cEvTextInput);
    connect_event_class(SDL_MOUSEBUTTONDOWN, cEvMouseButtonDown);
    connect_event_class(SDL_MOUSEBUTTONUP, cEvMouseButtonUp);
    connect_event_class(SDL_MOUSEMOTION, cEvMouseMotion);
    connect_event_class(SDL_MOUSEWHEEL, cEvMouseWheel);
    connect_event_class(SDL_JOYBUTTONDOWN, cEvJoyButtonDown);
    connect_event_class(SDL_JOYBUTTONUP, cEvJoyButtonUp);
    connect_event_class(SDL_JOYAXISMOTION, cEvJoyAxisMotion);
    connect_event_class(SDL_JOYBALLMOTION, cEvJoyBallMotion);
    connect_event_class(SDL_JOYDEVICEADDED, cEvJoyDeviceAdded);
    connect_event_class(SDL_JOYDEVICEREMOVED, cEvJoyDeviceRemoved);
    connect_event_class(SDL_JOYHATMOTION, cEvJoyHatMotion);
}

#define DEFINE_EVENT_READER(classname, classvar, name)                  \
    rb_define_method(classvar, #name, Ev##classname##_##name, 0)

#define DEFINE_EVENT_WRITER(classname, classvar, name)                  \
    rb_define_method(classvar, #name "=", Ev##classname##_set_##name, 1)

#define DEFINE_EVENT_ACCESSOR(classname, classvar, name)                \
    do {                                                                \
        DEFINE_EVENT_READER(classname, classvar, name);                 \
        DEFINE_EVENT_WRITER(classname, classvar, name);                 \
    } while(0)

void rubysdl2_init_event(void)
{
    cEvent = rb_define_class_under(mSDL2, "Event", rb_cObject);
    rb_define_alloc_func(cEvent, Event_s_allocate);
    rb_define_singleton_method(cEvent, "poll", Event_s_poll, 0);
    rb_define_singleton_method(cEvent, "enabled?", Event_s_enabled_p, 0);
    
    cEvQuit = rb_define_class_under(cEvent, "Quit", cEvent);
    cEvWindow = rb_define_class_under(cEvent, "Window", cEvent);
    cEvSysWM = rb_define_class_under(cEvent, "SysWM", cEvent);
    cEvKeyboard = rb_define_class_under(cEvent, "Keyboard", cEvent);
    cEvKeyUp = rb_define_class_under(cEvent, "KeyUp", cEvKeyboard);
    cEvKeyDown = rb_define_class_under(cEvent, "KeyDown", cEvKeyboard);
    cEvTextEditing = rb_define_class_under(cEvent, "TextEditing", cEvent);
    cEvTextInput = rb_define_class_under(cEvent, "TextInput", cEvent);
    cEvMouseButton = rb_define_class_under(cEvent, "MouseButton", cEvent);
    cEvMouseButtonDown = rb_define_class_under(cEvent, "MouseButtonDown", cEvMouseButton);
    cEvMouseButtonUp = rb_define_class_under(cEvent, "MouseButtonUp", cEvMouseButton);
    cEvMouseMotion = rb_define_class_under(cEvent, "MouseMotion", cEvent);
    cEvMouseWheel = rb_define_class_under(cEvent, "MouseWheel", cEvent);
    cEvJoyButton = rb_define_class_under(cEvent, "JoyButton", cEvent);
    cEvJoyButtonDown = rb_define_class_under(cEvent, "JoyButtonDown", cEvJoyButton);
    cEvJoyButtonUp = rb_define_class_under(cEvent, "JoyButtonUp", cEvJoyButton);
    cEvJoyAxisMotion = rb_define_class_under(cEvent, "JoyAxisMotion", cEvent);
    cEvJoyBallMotion = rb_define_class_under(cEvent, "JoyBallMotion", cEvent);
    cEvJoyHatMotion = rb_define_class_under(cEvent, "JoyHatMotion", cEvent);
    cEvJoyDevice = rb_define_class_under(cEvent, "JoyDevice", cEvent);
    cEvJoyDeviceAdded = rb_define_class_under(cEvent, "JoyDeviceAdded", cEvJoyDevice);
    cEvJoyDeviceRemoved = rb_define_class_under(cEvent, "JoyDeviceRemoved", cEvJoyDevice);

    DEFINE_EVENT_READER(Event, cEvent, type);
    DEFINE_EVENT_ACCESSOR(Event, cEvent, timestamp);
    rb_define_method(cEvent, "inspect", Event_inspect, 0);
    
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, window_id);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, event);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, data1);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, data2);
    rb_define_method(cEvWindow, "inspect", EvWindow_inspect, 0);

    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, window_id);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, state);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, repeat);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, scancode);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, sym);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, mod);
    rb_define_method(cEvKeyboard, "inspect", EvKeyboard_inspect, 0);
    
    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, window_id);
    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, text);
    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, start);
    DEFINE_EVENT_ACCESSOR(TextEditing, cEvTextEditing, length);
    rb_define_method(cEvTextEditing, "inspect", EvTextEditing_inspect, 0);
    
    DEFINE_EVENT_ACCESSOR(TextInput, cEvTextInput, window_id);
    DEFINE_EVENT_ACCESSOR(TextInput, cEvTextInput, text);
    rb_define_method(cEvTextInput, "inspect", EvTextInput_inspect, 0);

    DEFINE_EVENT_ACCESSOR(MouseButton, cEvMouseButton, window_id);
    DEFINE_EVENT_ACCESSOR(MouseButton, cEvMouseButton, which);
    DEFINE_EVENT_ACCESSOR(MouseButton, cEvMouseButton, button);
    DEFINE_EVENT_ACCESSOR(MouseButton, cEvMouseButton, pressed);
#if SDL_VERSION_ATLEAST(2,0,2)
    DEFINE_EVENT_ACCESSOR(MouseButton, cEvMouseButton, clicks);
#endif
    DEFINE_EVENT_ACCESSOR(MouseButton, cEvMouseButton, x);
    DEFINE_EVENT_ACCESSOR(MouseButton, cEvMouseButton, y);
    rb_define_alias(cEvMouseButton, "pressed?", "pressed");
    rb_define_method(cEvMouseButton, "inspect", EvMouseButton_inspect, 0);

    DEFINE_EVENT_ACCESSOR(MouseMotion, cEvMouseMotion, window_id);
    DEFINE_EVENT_ACCESSOR(MouseMotion, cEvMouseMotion, which);
    DEFINE_EVENT_ACCESSOR(MouseMotion, cEvMouseMotion, state);
    DEFINE_EVENT_ACCESSOR(MouseMotion, cEvMouseMotion, x);
    DEFINE_EVENT_ACCESSOR(MouseMotion, cEvMouseMotion, y);
    DEFINE_EVENT_ACCESSOR(MouseMotion, cEvMouseMotion, xrel);
    DEFINE_EVENT_ACCESSOR(MouseMotion, cEvMouseMotion, yrel);
    rb_define_method(cEvMouseMotion, "inspect", EvMouseMotion_inspect, 0);

    DEFINE_EVENT_ACCESSOR(MouseWheel, cEvMouseWheel, window_id);
    DEFINE_EVENT_ACCESSOR(MouseWheel, cEvMouseWheel, which);
    DEFINE_EVENT_ACCESSOR(MouseWheel, cEvMouseWheel, x);
    DEFINE_EVENT_ACCESSOR(MouseWheel, cEvMouseWheel, y);
    rb_define_method(cEvMouseWheel, "inspect", EvMouseWheel_inspect, 0);

    DEFINE_EVENT_ACCESSOR(JoyButton, cEvJoyButton, which);
    DEFINE_EVENT_ACCESSOR(JoyButton, cEvJoyButton, button);
    DEFINE_EVENT_ACCESSOR(JoyButton, cEvJoyButton, pressed);
    rb_define_alias(cEvJoyButton, "pressed?", "pressed");
    rb_define_method(cEvJoyButton, "inspect", EvJoyButton_inspect, 0);

    DEFINE_EVENT_ACCESSOR(JoyAxisMotion, cEvJoyAxisMotion, which);
    DEFINE_EVENT_ACCESSOR(JoyAxisMotion, cEvJoyAxisMotion, axis);
    DEFINE_EVENT_ACCESSOR(JoyAxisMotion, cEvJoyAxisMotion, value);
    rb_define_method(cEvJoyAxisMotion, "inspect", EvJoyAxisMotion_inspect, 0);

    DEFINE_EVENT_ACCESSOR(JoyBallMotion, cEvJoyBallMotion, which);
    DEFINE_EVENT_ACCESSOR(JoyBallMotion, cEvJoyBallMotion, ball);
    DEFINE_EVENT_ACCESSOR(JoyBallMotion, cEvJoyBallMotion, xrel);
    DEFINE_EVENT_ACCESSOR(JoyBallMotion, cEvJoyBallMotion, yrel);
    rb_define_method(cEvJoyBallMotion, "inspect", EvJoyBallMotion_inspect, 0);
    
    DEFINE_EVENT_ACCESSOR(JoyHatMotion, cEvJoyHatMotion, which);
    DEFINE_EVENT_ACCESSOR(JoyHatMotion, cEvJoyHatMotion, hat);
    DEFINE_EVENT_ACCESSOR(JoyHatMotion, cEvJoyHatMotion, value);
    rb_define_method(cEvJoyHatMotion, "inspect", EvJoyHatMotion_inspect, 0);
    
    DEFINE_EVENT_ACCESSOR(JoyDevice, cEvJoyDevice, which);
    rb_define_method(cEvJoyDevice, "inspect", EvJoyDevice_inspect, 0);
    
    init_event_type_to_class();
}
