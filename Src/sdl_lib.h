#pragma once
#include <unistd.h>

typedef struct SDL_Keysym
{
    __uint8_t scancode;      /**< SDL physical key code - see ::SDL_Scancode for details */
    __uint32_t sym;            /**< SDL virtual key code - see ::SDL_Keycode for details */
    __uint16_t mod;                 /**< current key modifiers */
    __uint32_t unused;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent
{
    __uint32_t type;        /**< ::SDL_KEYDOWN or ::SDL_KEYUP */
    __uint32_t timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    __uint32_t windowID;    /**< The window with keyboard focus, if any */
    __uint8_t state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
    __uint8_t repeat;       /**< Non-zero if this is a key repeat */
    __uint8_t padding2;
    __uint8_t padding3;
    SDL_Keysym keysym;  /**< The key that was pressed or released */
} SDL_KeyboardEvent;

typedef struct SDL_MouseMotionEvent
{
    __uint32_t type;        /**< ::SDL_MOUSEMOTION */
    __uint32_t timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    __uint32_t windowID;    /**< The window with mouse focus, if any */
    __uint32_t which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    __uint32_t state;       /**< The current button state */
    int x;           /**< X coordinate, relative to window */
    int y;           /**< Y coordinate, relative to window */
    int xrel;        /**< The relative motion in the X direction */
    int yrel;        /**< The relative motion in the Y direction */
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent
{
    __uint32_t type;        /**< ::SDL_MOUSEBUTTONDOWN or ::SDL_MOUSEBUTTONUP */
    __uint32_t timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    __uint32_t windowID;    /**< The window with mouse focus, if any */
    __uint32_t which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    __uint8_t button;       /**< The mouse button index */
    __uint8_t state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
    __uint8_t clicks;       /**< 1 for single-click, 2 for double-click, etc. */
    __uint8_t padding1;
    int x;           /**< X coordinate, relative to window */
    int y;           /**< Y coordinate, relative to window */
} SDL_MouseButtonEvent;

typedef union SDL_Event
{
    __uint32_t type;                            /**< Event type, shared with all events */
    SDL_KeyboardEvent key;                  /**< Keyboard event data */
    SDL_MouseMotionEvent motion;            /**< Mouse motion event data */
    SDL_MouseButtonEvent button;            /**< Mouse button event data */
    __uint8_t padding[56];

} SDL_Event;

struct SDL
{
    int scancode_offset = 93;
    __uint8_t SDL_RELEASED = 0;
    __uint8_t SDL_PRESSED = 1;
    __uint32_t SDL_KEYDOWN = 0x300;
    __uint32_t SDL_KEYUP = 0x301;
    __uint32_t SDL_MOUSEMOTION = 0x400;
    __uint32_t SDL_MOUSEBUTTONDOWN = 0x401;
    __uint32_t SDL_MOUSEBUTTONUP = 0x402;
    __uint32_t SDL_BUTTON_LEFT = 1;
    __uint32_t SDL_TOUCH_MOUSEID = ((__uint32_t)-1);

    __uint32_t KMOD_NONE = 0x0000;

    int (*SDL_PushEvent)(SDL_Event*);
    __uint32_t (*SDL_GetMouseState)(int *x, int *y);
};

struct SDL_keys
{
    __uint32_t SDLK_w = 0x0077;
    __uint32_t SDLK_s = 0x0073;
    __uint32_t SDLK_a = 0x0061;
    __uint32_t SDLK_d = 0x0064;
    __uint32_t SDLK_r = 0x0072;
    __uint32_t SDLK_q = 0x0071;
};
