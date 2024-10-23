#include "Environment_Interaction.h"

int Environment_Interaction::Keyboard_Event(__uint32_t key)
{
    SDL_Event event;
    event.type = this->sdl_util.SDL_KEYDOWN;
    event.key.type = this->sdl_util.SDL_KEYDOWN;
    event.key.state = this->sdl_util.SDL_PRESSED;
    event.key.repeat = 0;
    
    event.key.keysym.sym = key;
    event.key.keysym.scancode = key - this->sdl_util.scancode_offset;  // Scancode for the 'w' key
    event.key.keysym.mod = this->sdl_util.KMOD_NONE;  // No modifier keys like Shift, Ctrl, etc.
    
    return this->sdl_util.SDL_PushEvent(&event);
}

// dx and dy are movements on x,y plane of the mouse and also changes in yaw and pitch respectively
int Environment_Interaction::Mouse_Motion_Event(__uint32_t dx, __uint32_t dy)
{
    SDL_Event event;
    event.type = this->sdl_util.SDL_MOUSEMOTION;
    event.motion.type = this->sdl_util.SDL_MOUSEMOTION;
    event.motion.state = 0;

    this->sdl_util.SDL_GetMouseState(&event.motion.x, &(event.motion.y));
    event.motion.xrel = dy;
    event.motion.yrel = dx;

    return this->sdl_util.SDL_PushEvent(&event);
}

    // no arguments becuase were just telling the game to fire the gun
int Environment_Interaction:: Mouse_Button_Event()
{
    SDL_Event event;
    event.type = this->sdl_util.SDL_MOUSEBUTTONDOWN;
    
    event.button.type = this->sdl_util.SDL_MOUSEBUTTONDOWN;   // SDL_MOUSEBUTTONDOWN for press, SDL_MOUSEBUTTONUP for release
    event.button.button = this->sdl_util.SDL_BUTTON_LEFT;     // Specify the button (e.g., SDL_BUTTON_LEFT for left-click)
    event.button.state = this->sdl_util.SDL_PRESSED;          // SDL_PRESSED or SDL_RELEASED
    event.button.clicks = 1;                   // Number of clicks (1 for single, 2 for double click)
    this->sdl_util.SDL_GetMouseState(&event.motion.x, &(event.motion.y));    
    event.button.which = this->sdl_util.SDL_TOUCH_MOUSEID;    // For single touch or mouse events, SDL_TOUCH_MOUSEID
    
    return this->sdl_util.SDL_PushEvent(&event);
}

