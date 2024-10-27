#include "ac_detour.h"
#include "Environment_Interaction.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>

struct Data_Link
{
    __uint8_t *GameState[64]; // placeholders
    __uint8_t *GameAction[64];
};  

SDL_keys sdl_keys;

struct Hook_Util 
{
    __uint64_t victim_address;
    __uint64_t page_number;
    __uint8_t *original_instructions;
    void *handle;
    Environment_Interaction *interface;

}hook_util;

// super evil code >:) .. manipulates the internal event queue
void hook_function() {
    std::ofstream outFile("./ac_detour.log");

    int result = hook_util.interface->Mouse_Button_Event();

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
    __uint64_t return_address = hook_util.victim_address+AC_detour::injection_offset+AC_detour::hook_instruction_length;
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
    std::ofstream outFile("./ac_detour.log");

    hook_util.handle = dlopen("native_client", RTLD_LAZY | RTLD_NOLOAD);
    
    int shm_fd = shm_open("/ac_data_link", O_CREAT | O_RDWR, 0666);

    void *sdl_pushevent_address = dlsym(hook_util.handle, "SDL_PushEvent");
    void *sdl_getmousestate_address = dlsym(hook_util.handle, "SDL_GetMouseState");

    hook_util.interface = new Environment_Interaction;
    hook_util.interface->sdl_util.SDL_PushEvent = (int (*)(SDL_Event*))sdl_pushevent_address;
    hook_util.interface->sdl_util.SDL_GetMouseState = (__uint32_t (*)(int *x, int *y))sdl_getmousestate_address;

    AC_detour detour((__uint64_t)&trampoline_function);

    hook_util.victim_address = detour.victim_address;
    hook_util.page_number = detour.page_number;
    hook_util.original_instructions = detour.original_instructions;

    outFile << "successfully hooked";

    outFile.close();
}

void __attribute__((destructor)) unload()
{
    std::ofstream outFile("./ac_detour.log");
    //dlclose(hook_util.handle);
    mprotect((void*)hook_util.page_number, AC_detour::target_page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    std::memcpy((void*)(hook_util.victim_address+AC_detour::injection_offset), (void*)hook_util.original_instructions, AC_detour::hook_instruction_length);
    mprotect((void*)hook_util.page_number, AC_detour::target_page_size, PROT_READ | PROT_EXEC);
    
    outFile << "successfully unhooked\n";
    outFile.close();
}