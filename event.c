#include "rubysdl2_internal.h"
#include <SDL_events.h>

static VALUE cEvent;
static VALUE cKeyboardEvent;
static VALUE cKeyDown;
static VALUE cKeyUp;

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


#define EVENT_ACCESSOR(classname, name, field, ruby2c, c2ruby)  \
    static VALUE classname##_##name(VALUE self)                 \
    {                                                           \
        SDL_Event* ev;                                          \
        Data_Get_Struct(self, SDL_Event, ev);                   \
        return c2ruby(ev->field);                               \
    }                                                           \
    static VALUE classname##_set_##name(VALUE self, VALUE val)  \
    {                                                           \
        SDL_Event* ev;                                          \
        Data_Get_Struct(self, SDL_Event, ev);                   \
        ev->field = ruby2c(val);                                \
        return Qnil;                                            \
    }

#define EVENT_ACCESSOR_UINT(classname, name, field) \
    EVENT_ACCESSOR(classname, name, field, NUM2UINT, UINT2NUM)

EVENT_ACCESSOR_UINT(Event, timestamp, common.timestamp);
EVENT_ACCESSOR_UINT(Keyboard, state, key.state);
EVENT_ACCESSOR_UINT(Keyboard, scancode, key.keysym.scancode);
EVENT_ACCESSOR_UINT(Keyboard, sym, key.keysym.sym);
EVENT_ACCESSOR_UINT(Keyboard, mod, key.keysym.mod);

static void init_event_type_to_class(void)
{
    int i;
    for (i=0; i<SDL_LASTEVENT; ++i)
        event_type_to_class[i] = cEvent;
    
    event_type_to_class[SDL_KEYDOWN] = cKeyDown;
    event_type_to_class[SDL_KEYUP] = cKeyUp;
}

#define DEFINE_EVENT_ACCESSOR(classname, classvar, name)                \
    do {                                                                \
        rb_define_method(classvar, #name, classname##_##name, 0);       \
        rb_define_method(classvar, #name "=", classname##_set_##name, 1); \
    } while(0)

void rubysdl2_init_event(void)
{
    cEvent = rb_define_class_under(mSDL2, "Event", rb_cObject);
    rb_define_singleton_method(cEvent, "poll", Event_s_poll, 0);
    
    cKeyboardEvent = rb_define_class_under(cEvent, "Keyboard", cEvent);
    cKeyUp = rb_define_class_under(cEvent, "KeyUp", cKeyboardEvent);
    cKeyDown = rb_define_class_under(cEvent, "KeyDown", cKeyboardEvent);

    DEFINE_EVENT_ACCESSOR(Event, cEvent, timestamp);
    DEFINE_EVENT_ACCESSOR(Keyboard, cKeyboardEvent, state);
    DEFINE_EVENT_ACCESSOR(Keyboard, cKeyboardEvent, scancode);
    DEFINE_EVENT_ACCESSOR(Keyboard, cKeyboardEvent, sym);
    DEFINE_EVENT_ACCESSOR(Keyboard, cKeyboardEvent, mod);
    
    init_event_type_to_class();
}
