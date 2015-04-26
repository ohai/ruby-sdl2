#include "rubysdl2_internal.h"
#include <SDL_gamecontroller.h>

static VALUE cGameController;
static VALUE mAxis;
static VALUE mButton;

typedef struct GameController {
    SDL_GameController* controller;
} GameController;

static void GameController_free(GameController* g)
{
    if (rubysdl2_is_active() && g->controller)
        SDL_GameControllerClose(g->controller);
    free(g);
}

/*
 * Document-class: SDL2::GameController
 *
 * This class represents a "Game Controller".
 *
 * In SDL2, there is a gamecontroller framework to
 * hide the details of joystick types. This framework
 * is built on the existing joystick API.
 * 
 * The framework consists of two parts:
 *
 * * Mapping joysticks to game controllers
 * * Aquire input from game controllers
 *
 * Mapping information is a string, and described as:
 *
 *    GUID,name,mapping
 *
 * GUID is a unique ID of a type of joystick and
 * given by {Joystick#GUID}. name is the human readable
 * string for the device, and mappings are contorller mappings
 * to joystick ones.
 * 
 * This mappings abstract the details of joysticks, for exmaple,
 * the number of buttons, the number of axes, and the position
 * of these buttons a on pad.
 * You can use unified interface 
 * with this framework theoretically.
 * Howerver, we need to prepare the
 * database of many joysticks that users will use. A database
 * is available at https://github.com/gabomdq/SDL_GameControllerDB,
 * but perhaps is is not sufficient for your usage.
 * In fact, Steam prepares its own database for Steam games,
 * so if you will create a game for Steam, this framework is
 * useful. Otherwise, it is a better way to use joystick API
 * directly.
 * 
 */

static VALUE GameController_new(SDL_GameController* controller)
{
    GameController* g = ALLOC(GameController);
    g->controller = controller;
    return Data_Wrap_Struct(cGameController, 0, GameController_free, g);
}

DEFINE_WRAPPER(SDL_GameController, GameController, controller, cGameController,
               "SDL2::GameController");


/*
 * @overload add_mapping(string)
 *
 *   Add suuport for controolers that SDL is unaware of or to cause an
 *   existing controller to have a different binding.
 *
 *   "GUID,name,mapping", where GUID is
 *   the string value from SDL_JoystickGetGUIDString(), name is the human
 *   readable string for the device and mappings are controller mappings to
 *   joystick ones. Under Windows there is a reserved GUID of "xinput" that
 *   covers all XInput devices. The mapping format for joystick is:
 * 
 *   * bX:   a joystick button, index X
 *   * hX.Y: hat X with value Y
 *   * aX:   axis X of the joystick
 *
 *   Buttons can be used as a controller axes and vice versa.
 *
 *   This string shows an example of a valid mapping for a controller:
 *   "341a3608000000000000504944564944,Afterglow PS3 Controller,a:b1,b:b2,y:b3,x:b0,start:b9,guide:b12,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7"
 *   
 *   @param string [String] mapping string
 *   @return [Integer] 1 if a new mapping, 0 if an existing mapping is updated.
 */
static VALUE GameController_s_add_mapping(VALUE self, VALUE string)
{
    int ret = HANDLE_ERROR(SDL_GameControllerAddMapping(StringValueCStr(string)));
    return INT2NUM(ret);
}

/*
 * @overload axis_name_of(axis)
 *
 *   Get a string representing **axis**.
 *
 *   **axis** should be one of the following:
 *
 *   @param axis [Integer] axis constant
 *   @return [String]
 */
static VALUE GameController_s_axis_name_of(VALUE self, VALUE axis)
{
    const char* name = SDL_GameControllerGetStringForAxis(NUM2INT(axis));
    if (!name) {
        SDL_SetError("Unknown axis %d", NUM2INT(axis));
        SDL_ERROR();
    }
    return utf8str_new_cstr(name);
}

static VALUE GameController_s_button_name_of(VALUE self, VALUE button)
{
    const char* name = SDL_GameControllerGetStringForButton(NUM2INT(button));
    if (!name) {
        SDL_SetError("Unknown axis %d", NUM2INT(button));
        SDL_ERROR();
    }
    return utf8str_new_cstr(name);
}

static VALUE GameController_s_axis_from_name(VALUE self, VALUE name)
{
    int axis = SDL_GameControllerGetAxisFromString(StringValueCStr(name));
    if (axis < 0) {
        SDL_SetError("Unknown axis name \"%s\"", StringValueCStr(name));
        SDL_ERROR();
    } 
    return INT2FIX(axis);
}

static VALUE GameController_s_button_from_name(VALUE self, VALUE name)
{
    int button = SDL_GameControllerGetButtonFromString(StringValueCStr(name));
    if (button < 0) {
        SDL_SetError("Unknown button name \"%s\"", StringValueCStr(name));
        SDL_ERROR();
    }
    return INT2FIX(button);
}

static VALUE GameController_s_device_names(VALUE self)
{
    int num_joysticks = SDL_NumJoysticks();
    int i;
    VALUE device_names = rb_ary_new2(num_joysticks);
    for (i=0; i<num_joysticks; ++i) {
        const char* name = SDL_GameControllerNameForIndex(i);
        if (name)
            rb_ary_push(device_names, utf8str_new_cstr(name));
        else
            rb_ary_push(device_names, Qnil);
    }
    return device_names;
}

static VALUE GameController_s_mapping_for(VALUE self, VALUE guid_string)
{
    SDL_JoystickGUID guid = SDL_JoystickGetGUIDFromString(StringValueCStr(guid_string));
    return utf8str_new_cstr(SDL_GameControllerMappingForGUID(guid));
}

static VALUE GameController_s_open(VALUE self, VALUE index)
{
    SDL_GameController* controller = SDL_GameControllerOpen(NUM2INT(index));
    if (!controller)
        SDL_ERROR();
    return GameController_new(controller);
}

static VALUE GameController_name(VALUE self)
{
    return utf8str_new_cstr(SDL_GameControllerName(Get_SDL_GameController(self)));
}

static VALUE GameController_attached_p(VALUE self)
{
    return INT2BOOL(SDL_GameControllerGetAttached(Get_SDL_GameController(self)));
}

static VALUE GameController_destroy(VALUE self)
{
    GameController* g = Get_GameController(self);
    if (g->controller)
        SDL_GameControllerClose(g->controller);
    g->controller = NULL;
    return Qnil;
}

static VALUE GameController_mapping(VALUE self)
{
    return utf8str_new_cstr(SDL_GameControllerMapping(Get_SDL_GameController(self)));
}

static VALUE GameController_axis(VALUE self, VALUE axis)
{
    return INT2FIX(SDL_GameControllerGetAxis(Get_SDL_GameController(self),
                                             NUM2INT(axis)));
}

static VALUE GameController_button_pressed_p(VALUE self, VALUE button)
{
    return INT2BOOL(SDL_GameControllerGetButton(Get_SDL_GameController(self),
                                                NUM2INT(button)));
}

void rubysdl2_init_gamecontorller(void)
{
    cGameController = rb_define_class_under(mSDL2, "GameController", rb_cObject);

    rb_define_singleton_method(cGameController, "add_mapping",
                               GameController_s_add_mapping, 1);
    rb_define_singleton_method(cGameController, "device_names",
                               GameController_s_device_names, 0);
    rb_define_singleton_method(cGameController, "axis_name_of",
                               GameController_s_axis_name_of, 1);
    rb_define_singleton_method(cGameController, "button_name_of",
                               GameController_s_button_name_of, 1);
    rb_define_singleton_method(cGameController, "mapping_for",
                               GameController_s_mapping_for, 1);
    rb_define_singleton_method(cGameController, "button_from_name",
                               GameController_s_button_from_name, 1);
    rb_define_singleton_method(cGameController, "axis_from_name",
                               GameController_s_axis_from_name, 1);
    rb_define_singleton_method(cGameController, "open", GameController_s_open, 1);
    rb_define_method(cGameController, "destroy?", GameController_destroy_p, 0);
    rb_define_method(cGameController, "destroy", GameController_destroy, 0);
    rb_define_method(cGameController, "name", GameController_name, 0);
    rb_define_method(cGameController, "attached?", GameController_attached_p, 0);
    rb_define_method(cGameController, "mapping", GameController_mapping, 0);
    rb_define_method(cGameController, "axis", GameController_axis, 1);
    rb_define_method(cGameController, "button_pressed?", GameController_button_pressed_p, 1);

    mAxis = rb_define_module_under(cGameController, "Axis");
    mButton = rb_define_module_under(cGameController, "Button");
    
#define DEFINE_CONTROLLER_AXIS_CONST(type) \
    rb_define_const(mAxis, #type, INT2NUM(SDL_CONTROLLER_AXIS_##type))
    DEFINE_CONTROLLER_AXIS_CONST(INVALID);
    DEFINE_CONTROLLER_AXIS_CONST(LEFTX);
    DEFINE_CONTROLLER_AXIS_CONST(LEFTY);
    DEFINE_CONTROLLER_AXIS_CONST(RIGHTX);
    DEFINE_CONTROLLER_AXIS_CONST(RIGHTY);
    DEFINE_CONTROLLER_AXIS_CONST(TRIGGERLEFT);
    DEFINE_CONTROLLER_AXIS_CONST(TRIGGERRIGHT);
    DEFINE_CONTROLLER_AXIS_CONST(MAX);
#define DEFINE_CONTROLLER_BUTTON_CONST(type) \
    rb_define_const(mButton, #type, INT2NUM(SDL_CONTROLLER_BUTTON_##type))
    DEFINE_CONTROLLER_BUTTON_CONST(INVALID);
    DEFINE_CONTROLLER_BUTTON_CONST(A);
    DEFINE_CONTROLLER_BUTTON_CONST(B);
    DEFINE_CONTROLLER_BUTTON_CONST(X);
    DEFINE_CONTROLLER_BUTTON_CONST(Y);
    DEFINE_CONTROLLER_BUTTON_CONST(BACK);
    DEFINE_CONTROLLER_BUTTON_CONST(GUIDE);
    DEFINE_CONTROLLER_BUTTON_CONST(START);
    DEFINE_CONTROLLER_BUTTON_CONST(LEFTSTICK);
    DEFINE_CONTROLLER_BUTTON_CONST(RIGHTSTICK);
    DEFINE_CONTROLLER_BUTTON_CONST(LEFTSHOULDER);
    DEFINE_CONTROLLER_BUTTON_CONST(RIGHTSHOULDER);
    DEFINE_CONTROLLER_BUTTON_CONST(DPAD_UP);
    DEFINE_CONTROLLER_BUTTON_CONST(DPAD_DOWN);
    DEFINE_CONTROLLER_BUTTON_CONST(DPAD_LEFT);
    DEFINE_CONTROLLER_BUTTON_CONST(DPAD_RIGHT);
    DEFINE_CONTROLLER_BUTTON_CONST(MAX);
}
