#pragma once
#include "sdl_lib.h"
#include <unistd.h>

class Environment_Interaction
{
public:
    SDL sdl_util;

    int Keyboard_Event(__uint32_t key, bool isPressed);
    // dx and dy are movements on x,y plane of the mouse and also changes in yaw and pitch respectively
    int Mouse_Motion_Event(__uint32_t dx, __uint32_t dy);
    int Mouse_Button_Event(bool isPressed);
};