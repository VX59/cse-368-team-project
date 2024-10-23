#include "sdl_lib.h"
#include <unistd.h>

class Environment_Interaction
{
public:
    SDL sdl_util;

    int Keyboard_Event(__uint32_t key);
    // dx and dy are movements on x,y plane of the mouse and also changes in yaw and pitch respectively
    int Mouse_Motion_Event(__uint32_t dx, __uint32_t dy);
    // no arguments becuase were just telling the game to fire the gun
    int Mouse_Button_Event();
};