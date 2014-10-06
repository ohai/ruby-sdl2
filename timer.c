#include "rubysdl2_internal.h"
#include <SDL_timer.h>

static VALUE SDL_s_delay(VALUE self, VALUE ms)
{
    SDL_Delay(NUM2UINT(ms));
    return Qnil;
}

static VALUE SDL_s_get_ticks(VALUE self)
{
    return UINT2NUM(SDL_GetTicks());
}

static VALUE SDL_s_get_performance_counter(VALUE self)
{
    return UINT2NUM(SDL_GetPerformanceCounter());
}

static VALUE SDL_s_get_performance_frequency(VALUE self)
{
    return UINT2NUM(SDL_GetPerformanceFrequency());
}

void rubysdl2_init_timer(void)
{
    rb_define_module_function(mSDL2, "delay", SDL_s_delay, 1);
    rb_define_module_function(mSDL2, "get_ticks", SDL_s_get_ticks, 0);
    rb_define_module_function(mSDL2, "get_performance_counter",
                              SDL_s_get_performance_counter, 0);
    rb_define_module_function(mSDL2, "get_performance_frequency",
                              SDL_s_get_performance_frequency,0);
}
