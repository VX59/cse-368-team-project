#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <random>
#include <dlfcn.h>
#include "ac_detour.h"

typedef struct SDL_Keysym
{
    __uint8_t scancode;      /**< SDL physical key code - see ::SDL_Scancode for details */
    int32_t sym;            /**< SDL virtual key code - see ::SDL_Keycode for details */
    __uint16_t mod;                 /**< current key modifiers */
    __uint32_t unused;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent
{
    __uint32_t type;        /**< ::SDL_KEYDOWN or ::SDL_KEYUP */
    __uint32_t timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    __uint32_t windowID;    /**< The window with keyboard focus, if any */
    __uint8_t state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
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
    int32_t x;           /**< X coordinate, relative to window */
    int32_t y;           /**< Y coordinate, relative to window */
} SDL_MouseButtonEvent;

typedef union SDL_Event
{
    __uint32_t type;                            /**< Event type, shared with all events */
    char common[8];                         /**< Common event data */
    char display[20];               /**< Display event data */
    char window[24];                 /**< Window event data */
    SDL_KeyboardEvent key;                  /**< Keyboard event data */
    char edit[21];              /**< Text editing event data */
    char text[13];                /**< Text input event data */
    SDL_MouseMotionEvent motion;            /**< Mouse motion event data */
    SDL_MouseButtonEvent button;            /**< Mouse button event data */
    char wheel[28];              /**< Mouse wheel event data */
    char jaxis[20];                 /**< Joystick axis event data */
    char jball[20];                 /**< Joystick ball event data */
    char jhat[16];                   /**< Joystick hat event data */
    char jbutton[16];             /**< Joystick button event data */
    char jdevice[12];             /**< Joystick device change event data */
    char caxis[20];          /**< Game Controller axis event data */
    char cbutton[16];      /**< Game Controller button event data */
    char cdevice[12];      /**< Game Controller device event data */
    char ctouchpad[32];  /**< Game Controller touchpad event data */
    char csensor[28];      /**< Game Controller sensor event data */
    char adevice[16];           /**< Audio device event data */
    char sensor[36];                 /**< Sensor event data */
    char quit[8];                     /**< Quit request event data */
    char user[32];                     /**< Custom event data */
    char syswm[16];                   /**< System dependent window event data */
    char tfinger[48];           /**< Touch finger event data */
    char mgesture[36];         /**< Gesture event data */
    char dgesture[40];        /**< Gesture event data */
    char drop[13];                     /**< Drag and drop event data */

    /* This is necessary for ABI compatibility between Visual C++ and GCC
       Visual C++ will respect the push pack pragma and use 52 bytes for
       this structure, and GCC will use the alignment of the largest datatype
       within the union, which is 8 bytes.

       So... we'll add padding to force the size to be 56 bytes for both.
    */
    __uint8_t padding[56];
} SDL_Event;

void __attribute__((constructor)) init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/sdl_probe.log");
        
    __uint64_t pushevent_offset = 0x69840;


    outFile.close();
}


// maybe try loggin the events vector first
// void __attribute__((constructor)) init()
// {
//     std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/sdl_probe.log");

//     void *ac_handle = dlopen("native_client", RTLD_LAZY | RTLD_NOLOAD);

//     typedef int (*SDL_PushEvent_t)(SDL_Event*);
//     SDL_PushEvent_t SDL_PushEvent = (SDL_PushEvent_t)dlsym(ac_handle, "SDL_PushEvent");
    
//     SDL_Event event;
//     event.type = 0x300; // SDL_KEYDOWN
//     event.key.keysym.sym = ' ';
//     event.key.state = 1;

//     if (SDL_PushEvent)
//     {
//         SDL_PushEvent(&event);
//     }
//     else
//     {
//         outFile << "failed to resolve push event";
//     }

//     outFile.close();
// }
