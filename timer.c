#include "rubysdl2_internal.h"
#include <SDL_timer.h>

/*
 * @overload delay(ms)
 *   @param [Integer] ms the number of milliseconds to delay
 *
 * Wait a specified milliseconds.
 * 
 * This function stops all ruby threads. If you want to keep running other
 * threads, you should use Kernel.sleep instead of this function.
 * 
 * @return [nil]
 */
static VALUE SDL_s_delay(VALUE self, VALUE ms)
{
    SDL_Delay(NUM2UINT(ms));
    return Qnil;
}

/*
 * Return the number of milliseconds since {SDL2.init} is called.
 *
 * @return [Integer] milliseconds in 32bit unsigned value.
 */
static VALUE SDL_s_get_ticks(VALUE self)
{
    return UINT2NUM(SDL_GetTicks());
}

/*
 * Return the current value of the high resolution counter.
 * 
 * This method is typically used for profiling.
 * 
 * @return [Integer] the current counter value 
 * @see SDL2.get_performance_frequency
 */
static VALUE SDL_s_get_performance_counter(VALUE self)
{
    return ULL2NUM(SDL_GetPerformanceCounter());
}

/*
 * Return the frequency (count per second) of the high resolution counter.
 *
 * @return [Integer] a platform-specific count per second.
 * @see SDL2.get_performance_counter
 */
static VALUE SDL_s_get_performance_frequency(VALUE self)
{
    return ULL2NUM(SDL_GetPerformanceFrequency());
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
