#include "Environment_Interaction.h"

int Environment_Interaction::Keyboard_Event(__uint32_t key, bool isPressed)
{
    SDL_Event event;
    if (isPressed) {
        event.type = this->sdl_util.SDL_KEYDOWN;
        event.key.type = this->sdl_util.SDL_KEYDOWN;
        event.key.state = this->sdl_util.SDL_PRESSED;
    }
    else {
        event.type = this->sdl_util.SDL_KEYUP;
        event.key.type = this->sdl_util.SDL_KEYUP;
        event.key.state = this->sdl_util.SDL_RELEASED;
    }
    event.key.repeat = 0;
    event.key.keysym.sym = key;
    event.key.keysym.scancode = key - this->sdl_util.scancode_offset;
    event.key.keysym.mod = this->sdl_util.KMOD_NONE;
    
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
    event.motion.xrel = dx;
    event.motion.yrel = dy;

    return this->sdl_util.SDL_PushEvent(&event);
}

int Environment_Interaction::Mouse_Button_Event(bool isPressed)
{
    SDL_Event event;
    if (isPressed) {
        event.type = this->sdl_util.SDL_MOUSEBUTTONDOWN;
        event.button.type = this->sdl_util.SDL_MOUSEBUTTONDOWN;
        event.button.state = this->sdl_util.SDL_PRESSED;
    }
    else {
        event.type = this->sdl_util.SDL_MOUSEBUTTONUP;
        event.button.type = this->sdl_util.SDL_MOUSEBUTTONUP;
        event.button.state = this->sdl_util.SDL_RELEASED;
    }

    event.button.button = this->sdl_util.SDL_BUTTON_LEFT;
    event.button.clicks = 1;
    this->sdl_util.SDL_GetMouseState(&event.motion.x, &(event.motion.y));    
    event.button.which = this->sdl_util.SDL_TOUCH_MOUSEID;

    return this->sdl_util.SDL_PushEvent(&event);
}
