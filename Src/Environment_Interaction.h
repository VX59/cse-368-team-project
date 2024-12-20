#pragma once
#include "sdl_lib.h"
#include <unistd.h>

class Environment_Interaction
{
public:
    SDL sdl_util;
    SDL_keys sdl_keys;
    int Keyboard_Event(__uint32_t key, __uint32_t type, __uint32_t state);
    // dx and dy are movements on x,y plane of the mouse and also changes in yaw and pitch respectively
    int Mouse_Motion_Event(__uint32_t dx, __uint32_t dy);
    int Mouse_Button_Event(bool isPressed);
};