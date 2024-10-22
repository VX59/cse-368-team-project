#include <unistd.h>

struct hook_util {
    __uint64_t check_input_address;
    __uint64_t page_number;
    __uint8_t *original_instructions;
    void *handle;
} (hook_util);

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
    __uint32_t x;           /**< X coordinate, relative to window */
    __uint32_t y;           /**< Y coordinate, relative to window */
    __uint32_t xrel;        /**< The relative motion in the X direction */
    __uint32_t yrel;        /**< The relative motion in the Y direction */
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
    __uint32_t x;           /**< X coordinate, relative to window */
    __uint32_t y;           /**< Y coordinate, relative to window */
} SDL_MouseButtonEvent;

typedef union SDL_Event
{
    __uint32_t type;                            /**< Event type, shared with all events */
    SDL_KeyboardEvent key;                  /**< Keyboard event data */
    SDL_MouseMotionEvent motion;            /**< Mouse motion event data */
    SDL_MouseButtonEvent button;            /**< Mouse button event data */
    __uint8_t padding[56];

} SDL_Event;