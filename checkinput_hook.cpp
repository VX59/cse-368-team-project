#include "ac_detour.h"
#include <dlfcn.h>
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

#define SDL_KEYDOWN 0x300;
#define SDL_PRESSED 1;
#define SDLK_w 0x0077;
#define  SDL_SCANCODE_W 26;
#define KMOD_NONE 0x0000;
//__uint64_t events_offset = 0x144F60; // unused just keeping it around .. std::vector<SDL_Event> events is for like console and menu shit

// super evil code >:) ... force forwards movement .. pushes to the internal event queue
void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    SDL_Event event;

    event.type = SDL_KEYDOWN;
    event.key.type = SDL_KEYDOWN;
    event.key.state = SDL_PRESSED;
    event.key.repeat = 0;
    
    // Fill in the keysym structure with information about the "w" key
    event.key.keysym.sym = SDLK_w;  // SDLK_w corresponds to the 'w' key
    event.key.keysym.scancode = SDL_SCANCODE_W;  // Scancode for the 'w' key
    event.key.keysym.mod = KMOD_NONE;  // No modifier keys like Shift, Ctrl, etc.

    void *sdl_pushevent_address = dlsym(hook_util.handle, "SDL_PushEvent");
    outFile << std::hex << (__uint64_t)sdl_pushevent_address << std::endl;
    int (*SDL_PushEvent)(SDL_Event*) = (int (*)(SDL_Event*))sdl_pushevent_address;
    
    int result = SDL_PushEvent(&event);
    outFile << result << std::endl;
    if (result != 1) {
        outFile << "Failed to push event " << std::endl;
    } else {
        outFile << "Event pushed successfully." << std::endl;
    }
    outFile.close();
}

void __attribute__((naked)) trampoline_function()
{
    // execute stolen instructions
    __asm__(
        "mov %fs:0x28,%rax;"
        "mov %rax,0x1a0(%rsp);"
    );

    __uint64_t hook_function_address = (__uint64_t)&hook_function;
    __asm__(
        "call *%0;"
        :
        : "r" (hook_function_address)
    );

    // return controll flow to the game
    __uint64_t return_address = hook_util.check_input_address+AC_detour::injection_offset+AC_detour::hook_instruction_length;
    __asm__(
        "mov %0, %%rdx;"
        "jmp *%%rdx;"
        :
        : "r" (return_address)
        : "%rdx"
    );
}           

void __attribute__((constructor)) init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    AC_detour detour((__uint64_t)&trampoline_function);
    
    hook_util.handle = dlopen("native_client", RTLD_LAZY | RTLD_NOLOAD);
    hook_util.check_input_address = detour.check_input_address;
    hook_util.page_number = detour.page_number;
    hook_util.original_instructions = detour.original_instructions;

    outFile << "successfully hooked";

    outFile.close();
}

void __attribute__((destructor)) unload()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    dlclose(hook_util.handle);
    mprotect((void*)hook_util.page_number, AC_detour::target_page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    std::memcpy((void*)(hook_util.check_input_address+AC_detour::injection_offset), (void*)hook_util.original_instructions, AC_detour::hook_instruction_length);
    mprotect((void*)hook_util.page_number, AC_detour::target_page_size, PROT_READ | PROT_EXEC);
    
    outFile << "successfully unhooked\n";
    outFile.close();
}