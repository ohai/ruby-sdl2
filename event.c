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
static VALUE cEvControllerAxisMotion;
static VALUE cEvControllerButton;
static VALUE cEvControllerButtonDown;
static VALUE cEvControllerButtonUp;
static VALUE cEvControllerDevice;
static VALUE cEvControllerDeviceAdded;
static VALUE cEvControllerDeviceRemoved;
static VALUE cEvControllerDeviceRemapped;
static VALUE cEvTouchFinger;
static VALUE cEvFingerDown;
static VALUE cEvFingerUp;
static VALUE cEvFingerMotion;
/* static VALUE cEvUser; */
/* static VALUE cEvDrop; */

/* TODO:
   - easy
   SDL_FlushEvent{,s}
   SDL_HasEvent{,s}
   SDL_PumpEvent
   SDL_PeepEvents
   SDL_DropEvent
   SDL_QuitRequested
   - difficult
   SDL_PushEvent
   SDL_WaitEvent{,Timeout}
   SDL_UserEvent, SDL_RegisterEvents
 */

static VALUE event_type_to_class[SDL_LASTEVENT];

/*
 * Document-class: SDL2::Event
 *
 * This class represents SDL's events.
 *
 * All events are represented by the instance of subclasses of this class.
 *
 * # Introduction of event subsystem
 * Event handling allows your application to receive input from the user. Event handling is
 * initialized (along with video) with a call to:
 *
 *     SDL2.init(SDL2::INIT_VIDEO|SDL2::INIT_EVENTS)
 *
 * Internally, SDL stores all the events waiting to be handled in an event queue.
 * Using methods like {SDL2::Event.poll}, you can observe and handle input events.
 *
 * The queue is conceptually a sequence of objects of SDL2::Event.
 * You can read an event from the queue with {SDL2::Event.poll} and
 * you can process the information from the object.
 * 
 * Note: peep and wait will be implemented later.
 *
 * @attribute [rw] type
 *   SDL's internal event type enum
 *   @return [Integer] 
 *
 * @attribute [rw] timestamp
 *   timestamp of the event
 *   @return [Integer] 
 */
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

/*
 * Poll for currently pending events.
 *
 * @return [SDL2::Event] next event from the queue
 * @return [nil] the queue is empty
 */
static VALUE Event_s_poll(VALUE self)
{
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
        return Event_new(&ev);
    } else {
        return Qnil;
    }
}

/*
 * Get whether the event is enabled.
 *
 * This method is available for subclasses of SDL2::Event corresponding to
 * SDL's event types.
 *
 * @see .enabled= 
 */
static VALUE Event_s_enabled_p(VALUE self)
{
    VALUE event_type = rb_iv_get(self, "event_type");
    if (event_type == Qnil) {
        rb_warn("You cannot enable %s directly", rb_class2name(self));
        return Qfalse;
    }
    return INT2BOOL(SDL_EventState(NUM2INT(event_type), SDL_QUERY) == SDL_ENABLE);
}

/*
 * @overload enabled=(bool)
 *   Set wheter the event is enable
 *   
 *   This method is only available for subclasses of SDL2::Event corresponding to
 *   SDL's event types.
 *
 *   @example disable mouse wheel events
 *     SDL2::Event::MouseWheel.enable = false
 *   
 *   @param [Boolean] bool true for enabling the event.
 *   @return [bool]
 *   @see SDL2::Event.enabled?
 *
 */
static VALUE Event_s_set_enable(VALUE self, VALUE val)
{
    VALUE event_type = rb_iv_get(self, "event_type");
    if (event_type == Qnil) 
        rb_raise(rb_eArgError, "You cannot enable %s directly", rb_class2name(self));
    SDL_EventState(NUM2INT(event_type), RTEST(val) ? SDL_ENABLE : SDL_DISABLE);
    return val;
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

#define EVENT_ACCESSOR_DBL(classname, name, field) \
    EVENT_ACCESSOR(classname, name, field, NUM2DBL, DBL2NUM)

#define EVENT_ACCESSOR_BOOL(classname, name, field)             \
    EVENT_ACCESSOR(classname, name, field, RTEST, INT2BOOL)


EVENT_READER(Event, type, common.type, INT2NUM);
EVENT_ACCESSOR_UINT(Event, timestamp, common.timestamp);
/* @return [String] inspection string */
static VALUE Event_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp);
}

/*
 * Return the object of {SDL2::Window} corresponding to the window_id attribute.
 *
 * Some subclasses of SDL2::Event have window_id attribute to point the
 * window which creates the event. The type of the window_id attribute
 * is integer, and you need to convert it with {SDL2::Window.find_by_id}
 * to get the {SDL2::Window} object. This method returns the {SDL2::Window}
 * object.
 *
 * @return [SDL2::Window] the window which creates the event.
 * @raise [NoMethodError] raised if the window_id attribute is not present.
 */
static VALUE Event_window(VALUE self)
{
    VALUE window_id = rb_funcall(self, rb_intern("window_id"), 0, 0);
    return find_window_by_id(NUM2UINT(window_id));
}

/*
 * Document-class: SDL2::Event::Quit
 *
 * This class represents the quit requested event.
 *
 * This event occurs when a user try to close window, command-Q on OS X,
 * or any other quit request by a user. Normally if your application
 * receive this event, the application should exit. But the application
 * can query something to the users like "save", or ignore it.
 */

/*
 * Document-class: SDL2::Event::Window
 *
 * This class represents window event. This type of event occurs when
 * window state is changed.
 * 
 * @attribute [rw] window_id
 *   the associate window id
 *   @return [Integer]
 *
 * @attribute [rw] event
 *   event type, one of the following:
 *
 *   * SDL2::Event::Window::NONE - (never used)
 *   * SDL2::Event::Window::SHOWN - window has been shown
 *   * SDL2::Event::Window::HIDDEN - window has been hidden
 *   * SDL2::Event::Window::EXPOSED - window has been exposed and should be redrawn
 *   * SDL2::Event::Window::MOVED - window has been moved to data1, data2
 *   * SDL2::Event::Window::RESIZED - window has been resized to data1xdata2;
 *       this is event is always preceded by SIZE_CHANGED
 *   * SDL2::Event::Window::SIZE_CHANGED -
 *       window size has changed, either as a result of an API call or through
 *       the system or user changing the window size;
 *       this event is followed by
 *       RESIZED if the size was changed by an external event,
 *       i.e. the user or the window manager
 *   * SDL2::Event::Window::MINIMIZED - window has been minimized
 *   * SDL2::Event::Window::MAXIMIZED - window has been maximized
 *   * SDL2::Event::Window::RESTORED - window has been restored to normal size and position
 *   * SDL2::Event::Window::ENTER - window has gained mouse focus
 *   * SDL2::Event::Window::LEAVE - window has lost mouse focus
 *   * SDL2::Event::Window::FOCUS_GAINED - window has gained keyboard focus
 *   * SDL2::Event::Window::FOCUS_LOST -window has lost keyboard focus
 *   * SDL2::Event::Window::CLOSE - 
 *       the window manager requests that the window be closed
 *   
 *   @return [Integer]
 *
 * @attribute data1
 *   event dependent data
 *   @return [Integer]
 *
 * @attribute data2
 *   event dependent data
 *   @return [Integer]
 *
 */
EVENT_ACCESSOR_UINT(Window, window_id, window.windowID);
EVENT_ACCESSOR_UINT(Window, event, window.event);
EVENT_ACCESSOR_INT(Window, data1, window.data1);
EVENT_ACCESSOR_INT(Window, data2, window.data2);

/* @return [String] inspection string */
static VALUE EvWindow_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u window_id=%u event=%u data1=%d data2=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->window.windowID, ev->window.event,
                      ev->window.data1, ev->window.data2);
}

/*
 * Document-class: SDL2::Event::Keyboard
 *
 * This class represents keyboard event.
 *
 * You don't handle the instance 
 * of this class directly, but you handle the instances of 
 * two subclasses of this subclasses:
 * {SDL2::Event::KeyDown} and {SDL2::Event::KeyUp}.
 *
 * @attribute [rw] window_id
 *   the associate window id
 *   @return [Integer]
 *
 * @attribute pressed
 *   key is pressed
 *   @return [Integer]
 *
 * @attribute repeat
 *   key repeat
 *   @return [Integer]
 *   
 * @attribute scancode
 *   physical key code
 *   @return [Integer]
 *   @see SDL2::Key::Scan
 *
 * @attribute sym
 *   virtual key code
 *   @return [Integer]
 *   @see SDL2::Key
 *
 * @attribute mod
 *   current key modifier
 *   @return [Integer]
 *   @see SDL2::Key::Mod
 *   
 */
EVENT_ACCESSOR_UINT(Keyboard, window_id, key.windowID);
EVENT_ACCESSOR_BOOL(Keyboard, pressed, key.state);
EVENT_ACCESSOR_BOOL(Keyboard, repeat, key.repeat);
EVENT_ACCESSOR_UINT(Keyboard, scancode, key.keysym.scancode);
EVENT_ACCESSOR_UINT(Keyboard, sym, key.keysym.sym);
EVENT_ACCESSOR_UINT(Keyboard, mod, key.keysym.mod);
/* @return [String] inspection string */
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


/*
 * Document-class: SDL2::Event::KeyDown
 *
 * This class represents a key down event.
 */

/*
 * Document-class: SDL2::Event::KeyUp
 *
 * This class represents a key up event.
 */

/*
 * Document-class: SDL2::Event::TextEditing
 *
 * This class represents text editing event.
 *
 * @attribute window_id
 *   the associate window id
 *   @return [Integer]
 *
 * @attribute text
 *   the editing text
 *   @return [String]
 *
 * @attribute start
 *   the start cursor of selected editing text
 *   @return [Integer]
 *
 * @attribute length
 *   the length of selected editing text
 *   @return [Integer]
 */
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

/* @return [String] inspection string */
static VALUE EvTextEditing_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " window_id=%u text=%s start=%d length=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->edit.windowID, ev->edit.text, ev->edit.start, ev->edit.length);
}


/*
 * Document-class: SDL2::Event::TextInput
 *
 * This class represents text input events.
 *
 * @attribute window_id
 *   the associate window id
 *   @return [Integer]
 *
 * @attribute text
 *   the input text
 *   @return [String]
 */
EVENT_ACCESSOR_UINT(TextInput, window_id, text.windowID);
EVENT_READER(TextInput, text, text.text, utf8str_new_cstr);
static VALUE EvTextInput_set_text(VALUE self, VALUE str)
{
    SDL_Event* ev;
    Data_Get_Struct(self, SDL_Event, ev);
    set_string(ev->text.text, str, 30);
    return str;
}

/* @return [String] inspection string */
static VALUE EvTextInput_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u window_id=%u text=%s>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->text.windowID, ev->text.text);
}

/*
 * Document-class: SDL2::Event::MouseButton
 *
 * This class represents mouse button events.
 * 
 * You don't handle the instance 
 * of this class directly, but you handle the instances of 
 * two subclasses of this subclasses:
 * {SDL2::Event::MouseButtonDown} and {SDL2::Event::MouseButtonUp}.
 *
 * @attribute window_id
 *   the window id with mouse focus
 *   @return [Integer]
 *
 * @attribute which
 *   the mouse index
 *   @return [Integer]
 *
 * @attribute button
 *   the mouse button index
 *   @return [Integer]
 *
 * @attribute pressed
 *   button is pressed or not
 *   @return [Boolean]
 *
 * @attribute clicks
 *   1 for single click, 2 for double click
 *   
 *   This attribute is available after SDL 2.0.2
 *   @return [Integer]
 *
 * @attribute x
 *   the x coordinate of the mouse pointer, relative to window
 *   @return [Integer]
 *
 * @attribute y
 *   the y coordinate of the mouse pointer, relative to window
 *   @return [Integer]
 */
EVENT_ACCESSOR_UINT(MouseButton, window_id, button.windowID);
EVENT_ACCESSOR_UINT(MouseButton, which, button.which);
EVENT_ACCESSOR_UINT8(MouseButton, button, button.button);
EVENT_ACCESSOR_BOOL(MouseButton, pressed, button.state);
#if SDL_VERSION_ATLEAST(2,0,2)
EVENT_ACCESSOR_UINT8(MouseButton, clicks, button.clicks);
#endif
EVENT_ACCESSOR_INT(MouseButton, x, button.x);
EVENT_ACCESSOR_INT(MouseButton, y, button.y);
/* @return [String] inspection string */
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
                      ev->button.button, INT2BOOLCSTR(ev->button.state),
#if SDL_VERSION_ATLEAST(2,0,2)
                      ev->button.clicks,
#endif
                      ev->button.x, ev->button.y);
}

/*
 * Document-class: SDL2::Event::MouseButtonDown
 * This class represents mouse button press events.
 */
/*
 * Document-class: SDL2::Event::MouseButtonUp
 * This class represents mouse button release events.
 */

/*
 * Document-class: SDL2::Event::MouseMotion
 *
 * This class represents mouse motion events.
 *
 * @attribute window_id
 *   the window id with mouse focus
 *   @return [Integer]
 *
 * @attribute which
 *   the mouse index 
 *   @return [Integer]
 *
 * @attribute state
 *   the current mouse state
 *   @return [Integer]
 *   @todo use SDL2::Mouse::State
 *   
 * @attribute x
 *   the x coordinate of the mouse pointer, relative to window
 *   @return [Integer]
 *
 * @attribute y
 *   the y coordinate of the mouse pointer, relative to window
 *   @return [Integer]
 *
 * @attribute xrel
 *   the relative motion in the x direction
 *   @return [Integer]
 *
 * @attribute yrel
 *   the relative motion in the y direction
 *   @return [Integer]
 */
EVENT_ACCESSOR_UINT(MouseMotion, window_id, motion.windowID);
EVENT_ACCESSOR_UINT(MouseMotion, which, motion.which);
EVENT_ACCESSOR_UINT(MouseMotion, state, motion.state);
EVENT_ACCESSOR_INT(MouseMotion, x, motion.x);
EVENT_ACCESSOR_INT(MouseMotion, y, motion.y);
EVENT_ACCESSOR_INT(MouseMotion, xrel, motion.xrel);
EVENT_ACCESSOR_INT(MouseMotion, yrel, motion.yrel);
/* @return [String] inspection string */
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

/*
 * Document-class: SDL2::Event::MouseWheel
 *
 * This class represents mouse wheel events.
 *
 * @attribute window_id
 *   the window id with mouse focus
 *   @return [Integer]
 *
 * @attribute which
 *   the mouse index 
 *   @return [Integer]
 *
 * @attribute x
 *   the amount of scrolled horizontally, positive to the right and negative
 *   to the left
 *   @return [Integer]
 *
 * @attribute y
 *   the amount of scrolled vertically, positve away from the user and negative
 *   toward the user
 *   @return [Integer]
 */
EVENT_ACCESSOR_UINT(MouseWheel, window_id, wheel.windowID);
EVENT_ACCESSOR_UINT(MouseWheel, which, wheel.which);
EVENT_ACCESSOR_INT(MouseWheel, x, wheel.x);
EVENT_ACCESSOR_INT(MouseWheel, y, wheel.y);
/* @return [String] inspection string */
static VALUE EvMouseWheel_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " window_id=%u which=%u x=%d y=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->wheel.windowID, ev->wheel.which, ev->wheel.x, ev->wheel.y);
}

/*
 * Document-class: SDL2::Event::JoyButton
 *
 * This class represents joystick button events.
 *
 * You don't handle the instance 
 * of this class directly, but you handle the instances of 
 * two subclasses of this subclasses:
 * {SDL2::Event::JoyButtonDown} and {SDL2::Event::JoyButtonUp}.
 *
 * @attribute which
 *   the joystick index
 *   @return [Integer]
 *
 * @attribute button
 *   the joystick button index
 *   @return [Integer]
 *
 * @attribute pressed
 *   button is pressed or not
 *   @return [Integer]
 * 
 */
EVENT_ACCESSOR_INT(JoyButton, which, jbutton.which);
EVENT_ACCESSOR_UINT8(JoyButton, button, jbutton.button);
EVENT_ACCESSOR_BOOL(JoyButton, pressed, jbutton.state)
/* @return [Stirng] inspection string */
static VALUE EvJoyButton_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d button=%u pressed=%s>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jbutton.which, ev->jbutton.button,
                      INT2BOOLCSTR(ev->jbutton.state));
}
/*
 * Document-class: SDL2::Event::JoyButtonDown
 *
 * This class represents the joystick button press events.
 */

/*
 * Document-class: SDL2::Event::JoyButtonUp
 *
 * This class represents the joystick button release events.
 */

/*
 * Document-class: SDL2::Event::JoyAxisMotion
 *
 * This class represents the joystick axis motion events.
 *
 * @attribute which
 *   the joystick index
 *   @return [Integer]
 *
 * @attribute axis
 *   the axis index
 *   @return [Integer]
 *
 * @attribute value
 *   the axis value (range: -32768 to -32767, 0 for newtral)
 *   @return [Integer]
 */
EVENT_ACCESSOR_INT(JoyAxisMotion, which, jaxis.which);
EVENT_ACCESSOR_UINT8(JoyAxisMotion, axis, jaxis.axis);
EVENT_ACCESSOR_INT(JoyAxisMotion, value, jaxis.value);
/* @return [Stirng] inspection string */
static VALUE EvJoyAxisMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d axis=%u value=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jaxis.which, ev->jaxis.axis, ev->jaxis.value);
}

/*
 * Document-class: SDL2::Event::JoyBallMotion
 *
 * This class represents the joystick trackball motion events.
 *
 * @attribute which
 *   the joystick index
 *   @return [Integer]
 *
 * @attribute ball
 *   the joystick trackball index
 *   @return [Integer]
 *
 * @attribute xrel
 *   the relative motion in the x direction
 *   @return [Integer]
 *
 * @attribute yrel
 *   the relative motion in the y direction
 *   @return [Integer]
 */
EVENT_ACCESSOR_INT(JoyBallMotion, which, jball.which);
EVENT_ACCESSOR_UINT8(JoyBallMotion, ball, jball.ball);
EVENT_ACCESSOR_INT(JoyBallMotion, xrel, jball.xrel);
EVENT_ACCESSOR_INT(JoyBallMotion, yrel, jball.yrel);
/* @return [String] inspection string */
static VALUE EvJoyBallMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d ball=%u xrel=%d yrel=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jball.which, ev->jball.ball, ev->jball.xrel, ev->jball.yrel);
}

/*
 * Document-class: SDL2::Event::JoyHatMotion
 *
 * This class represents the joystick hat position change events.
 *
 * @attribute which
 *   the joystick index
 *   @return [Integer]
 *
 * @attribute hat
 *   the joystick hat index
 *   @return [Integer]
 *
 * @attribute value
 *   the hat position value, same value as {SDL2::Joystick#hat}.
 *   @return [Integer]
 */
EVENT_ACCESSOR_INT(JoyHatMotion, which, jhat.which);
EVENT_ACCESSOR_UINT8(JoyHatMotion, hat, jhat.hat);
EVENT_ACCESSOR_UINT8(JoyHatMotion, value, jhat.value);
/* @return [String] inspection string */
static VALUE EvJoyHatMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u which=%d hat=%u value=%u>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jhat.which, ev->jhat.hat, ev->jhat.value);
}

/*
 * Document-class: SDL2::Event::JoyDevice
 *
 * This class represents joystick device events
 * ({SDL2::Event::JoyDeviceAdded joystick connected events} and
 *  {SDL2::Event::JoyDeviceRemoved joystick disconnected events}).
 */
EVENT_ACCESSOR_INT(JoyDevice, which, jdevice.which);
static VALUE EvJoyDevice_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u which=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->jdevice.which);
}

/*
 * Document-class: SDL2::Event::JoyDeviceAdded
 *
 * This class represents joystick device connected events.
 */
/*
 * Document-class: SDL2::Event::JoyDeviceRemoved
 *
 * This class represents joystick device disconnected events.
 */


EVENT_ACCESSOR_INT(ControllerAxis, which, caxis.which);
EVENT_ACCESSOR_UINT8(ControllerAxis, axis, caxis.axis);
EVENT_ACCESSOR_INT(ControllerAxis, value, caxis.value);
static VALUE ControllerAxis_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d axis=%s value=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->caxis.which, SDL_GameControllerGetStringForAxis(ev->caxis.axis),
                      ev->caxis.value);
}

EVENT_ACCESSOR_INT(ControllerButton, which, cbutton.which);
EVENT_ACCESSOR_UINT8(ControllerButton, button, cbutton.button);
EVENT_ACCESSOR_BOOL(ControllerButton, pressed, cbutton.state);
static VALUE ControllerButton_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " which=%d button=%s state=%s>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->cbutton.which,
                      SDL_GameControllerGetStringForButton(ev->cbutton.button),
                      INT2BOOLCSTR(ev->cbutton.state));
}

EVENT_ACCESSOR_INT(ControllerDevice, which, cdevice.which);
static VALUE ControllerDevice_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u which=%d>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      ev->cdevice.which);
}

/*
 * Document-class: SDL2::Event::TouchFinger
 *
 * This class represents touch finger events.
 * 
 * You don't handle the instance 
 * of this class directly, but you handle the instances of 
 * two subclasses of this subclasses:
 * {SDL2::Event::FingerMotion} and {SDL2::Event::MouseButtonUp}.
 *
 * @attribute touch_id
 *   the touch device id
 *   @return [Integer]
 *
 * @attribute finger_id
 *   the finger id
 *   @return [Integer]
 *
 * @attribute x
 *   the x-axis location of the touch event, normalized (0...1)
 *   @return [Float]
 *
 * @attribute y
 *   the y-axis location of the touch event, normalized (0...1)
 *   @return [Float]
 *
 * @attribute pressure
 *   the quantity of pressure applied, normalized (0...1)
 *   @return [Float]
 */
EVENT_ACCESSOR_INT(TouchFinger, touch_id, tfinger.touchId);
EVENT_ACCESSOR_INT(TouchFinger, finger_id, tfinger.fingerId);
EVENT_ACCESSOR_DBL(TouchFinger, x, tfinger.x);
EVENT_ACCESSOR_DBL(TouchFinger, y, tfinger.y);
EVENT_ACCESSOR_DBL(TouchFinger, pressure, tfinger.pressure);
/* @return [String] inspection string */
static VALUE EvTouchFinger_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " touch_id=%d finger_id=%d"
                      " x=%f y=%f pressure=%f>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      (int)ev->tfinger.touchId, (int)ev->tfinger.fingerId,
                      ev->tfinger.x, ev->tfinger.y, ev->tfinger.pressure);
}

/*
 * Document-class: SDL2::Event::FingerUp
 * This class represents finger touch events.
 */
/*
 * Document-class: SDL2::Event::FingerDown
 * This class represents finger release events.
 */

/*
 * Document-class: SDL2::Event::FingerMove
 *
 * This class represents touch move events.
 *
 * @attribute dx
 *   the distance moved in the x-axis, normalized (0...1)
 *   @return [Float]
 *
 * @attribute dy
 *   the distance moved in the x-axis, normalized (0...1)
 *   @return [Float]
 *
 */
EVENT_ACCESSOR_DBL(FingerMotion, dx, tfinger.dx);
EVENT_ACCESSOR_DBL(FingerMotion, dy, tfinger.dy);
/* @return [String] inspection string */
static VALUE EvFingerMotion_inspect(VALUE self)
{
    SDL_Event* ev; Data_Get_Struct(self, SDL_Event, ev);
    return rb_sprintf("<%s: type=%u timestamp=%u"
                      " touch_id=%d finger_id=%d"
                      " x=%f y=%f pressure=%f"
                      " dy=%f dx=%f>",
                      rb_obj_classname(self), ev->common.type, ev->common.timestamp,
                      (int) ev->tfinger.touchId, (int) ev->tfinger.fingerId,
                      ev->tfinger.x, ev->tfinger.y, ev->tfinger.pressure,
                      ev->tfinger.dx, ev->tfinger.dx);
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
    connect_event_class(SDL_CONTROLLERAXISMOTION, cEvControllerAxisMotion);
    connect_event_class(SDL_CONTROLLERBUTTONDOWN, cEvControllerButtonDown);
    connect_event_class(SDL_CONTROLLERBUTTONUP, cEvControllerButtonUp);
    connect_event_class(SDL_CONTROLLERDEVICEADDED, cEvControllerDeviceAdded);
    connect_event_class(SDL_CONTROLLERDEVICEREMOVED, cEvControllerDeviceRemoved);
    connect_event_class(SDL_CONTROLLERDEVICEREMAPPED, cEvControllerDeviceRemapped);
    connect_event_class(SDL_FINGERDOWN, cEvFingerDown);
    connect_event_class(SDL_FINGERUP, cEvFingerUp);
    connect_event_class(SDL_FINGERMOTION, cEvFingerMotion);
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
    rb_define_singleton_method(cEvent, "enable=", Event_s_set_enable, 1);
    
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
    cEvControllerAxisMotion = rb_define_class_under(cEvent, "ControllerAxisMotion", cEvent);
    cEvControllerButton = rb_define_class_under(cEvent, "ControllerButton", cEvent);
    cEvControllerButtonDown = rb_define_class_under(cEvent, "ControllerButtonDown", cEvControllerButton);
    cEvControllerButtonUp = rb_define_class_under(cEvent, "ControllerButtonUp", cEvControllerButton);
    cEvControllerDevice = rb_define_class_under(cEvent, "ControllerDevice", cEvent);
    cEvControllerDeviceAdded = rb_define_class_under(cEvent, "ControllerDeviceAdded", cEvControllerDevice);
    cEvControllerDeviceRemoved = rb_define_class_under(cEvent, "ControllerDeviceRemoved", cEvControllerDevice);
    cEvControllerDeviceRemapped = rb_define_class_under(cEvent, "ControllerDeviceRemapped", cEvControllerDevice);
    cEvTouchFinger = rb_define_class_under(cEvent, "TouchFinger", cEvent);
    cEvFingerUp = rb_define_class_under(cEvent, "FingerUp", cEvTouchFinger);
    cEvFingerDown = rb_define_class_under(cEvent, "FingerDown", cEvTouchFinger);
    cEvFingerMotion = rb_define_class_under(cEvent, "FingerMotion", cEvTouchFinger);
    
    
    
    DEFINE_EVENT_READER(Event, cEvent, type);
    DEFINE_EVENT_ACCESSOR(Event, cEvent, timestamp);
    rb_define_method(cEvent, "inspect", Event_inspect, 0);
    rb_define_method(cEvent, "window", Event_window, 0);
    
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, window_id);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, event);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, data1);
    DEFINE_EVENT_ACCESSOR(Window, cEvWindow, data2);
    rb_define_method(cEvWindow, "inspect", EvWindow_inspect, 0);
#define DEFINE_EVENT_ID_CONST(t) \
    rb_define_const(cEvWindow, #t, INT2NUM(SDL_WINDOWEVENT_##t))
    DEFINE_EVENT_ID_CONST(NONE);
    DEFINE_EVENT_ID_CONST(SHOWN);
    DEFINE_EVENT_ID_CONST(HIDDEN);
    DEFINE_EVENT_ID_CONST(EXPOSED);
    DEFINE_EVENT_ID_CONST(MOVED);
    DEFINE_EVENT_ID_CONST(RESIZED);
    DEFINE_EVENT_ID_CONST(SIZE_CHANGED);
    DEFINE_EVENT_ID_CONST(MINIMIZED);
    DEFINE_EVENT_ID_CONST(MAXIMIZED);
    DEFINE_EVENT_ID_CONST(RESTORED);
    DEFINE_EVENT_ID_CONST(ENTER);
    DEFINE_EVENT_ID_CONST(LEAVE);
    DEFINE_EVENT_ID_CONST(FOCUS_GAINED);
    DEFINE_EVENT_ID_CONST(FOCUS_LOST);
    DEFINE_EVENT_ID_CONST(CLOSE);
    
                    
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, window_id);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, pressed);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, repeat);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, scancode);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, sym);
    DEFINE_EVENT_ACCESSOR(Keyboard, cEvKeyboard, mod);
    rb_define_alias(cEvKeyboard, "pressed?", "pressed");
    rb_define_alias(cEvKeyboard, "repeat?", "repeat");
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

    DEFINE_EVENT_ACCESSOR(ControllerAxis, cEvControllerAxisMotion, which);
    DEFINE_EVENT_ACCESSOR(ControllerAxis, cEvControllerAxisMotion, axis);
    DEFINE_EVENT_ACCESSOR(ControllerAxis, cEvControllerAxisMotion, value);
    rb_define_method(cEvControllerAxisMotion, "inspect", ControllerAxis_inspect, 0);

    DEFINE_EVENT_ACCESSOR(ControllerButton, cEvControllerButton, which);
    DEFINE_EVENT_ACCESSOR(ControllerButton, cEvControllerButton, button);
    DEFINE_EVENT_ACCESSOR(ControllerButton, cEvControllerButton, pressed);
    rb_define_method(cEvControllerButton, "inspect", ControllerButton_inspect, 0);

    DEFINE_EVENT_ACCESSOR(ControllerDevice, cEvControllerDevice, which);
    rb_define_method(cEvControllerDevice, "inspect", ControllerDevice_inspect, 0);

    DEFINE_EVENT_ACCESSOR(TouchFinger, cEvTouchFinger, touch_id);
    DEFINE_EVENT_ACCESSOR(TouchFinger, cEvTouchFinger, finger_id);
    DEFINE_EVENT_ACCESSOR(TouchFinger, cEvTouchFinger, x); 
    DEFINE_EVENT_ACCESSOR(TouchFinger, cEvTouchFinger, y);
    DEFINE_EVENT_ACCESSOR(TouchFinger, cEvTouchFinger, pressure);
    rb_define_method(cEvTouchFinger, "inspect", EvTouchFinger_inspect, 0);

    DEFINE_EVENT_ACCESSOR(FingerMotion, cEvFingerMotion, dx); 
    DEFINE_EVENT_ACCESSOR(FingerMotion, cEvFingerMotion, dy);
    rb_define_method(cEvFingerMotion, "inspect", EvFingerMotion_inspect, 0);
    
    init_event_type_to_class();
}
