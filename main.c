#include "rubysdl2_internal.h"
#include <SDL.h>

void Init_sdl2_ext(void)
{
  rubysdl2_init_video();
  return;
}
