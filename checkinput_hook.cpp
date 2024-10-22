#include "ac_detour.h"
#include "sdl_util.h"
#include <dlfcn.h>

struct SDL
{
    __uint8_t SDL_PRESSED = 1;
    __uint8_t  SDL_SCANCODE_W = 26;
    __uint32_t SDL_KEYDOWN = 0x300;
    __uint32_t SDLK_w = 0x0077;
    __uint32_t KMOD_NONE = 0x0000;
    int (*SDL_PushEvent)(SDL_Event*);
} (sdl_util);

struct hook_util 
{
    __uint64_t check_input_address;
    __uint64_t page_number;
    __uint8_t *original_instructions;
    void *handle;
    SDL sdl;
} (hook_util);

// super evil code >:) ... force forwards movement .. pushes to the internal event queue
void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    SDL_Event event;

    event.type = hook_util.sdl.SDL_KEYDOWN;
    event.key.type = hook_util.sdl.SDL_KEYDOWN;
    event.key.state = hook_util.sdl.SDL_PRESSED;
    event.key.repeat = 0;
    
    // Fill in the keysym structure with information about the "w" key
    event.key.keysym.sym = hook_util.sdl.SDLK_w;  // SDLK_w corresponds to the 'w' key
    event.key.keysym.scancode = hook_util.sdl.SDL_SCANCODE_W;  // Scancode for the 'w' key
    event.key.keysym.mod = hook_util.sdl.KMOD_NONE;  // No modifier keys like Shift, Ctrl, etc.
    
    int result = hook_util.sdl.SDL_PushEvent(&event);
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

    hook_util.sdl = sdl_util;
    hook_util.handle = dlopen("native_client", RTLD_LAZY | RTLD_NOLOAD);
    void *sdl_pushevent_address = dlsym(hook_util.handle, "SDL_PushEvent");
    hook_util.sdl.SDL_PushEvent = (int (*)(SDL_Event*))sdl_pushevent_address;

    AC_detour detour((__uint64_t)&trampoline_function);
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