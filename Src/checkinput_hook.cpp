#include "ac_detour.h"
#include "Environment_Interaction.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include "agents/conditional/conditional_agent.h"
#include "agents/math_helpers.h"
#include "hunter_agent.h"

SDL_keys sdl_keys;
Features features;
Hunter_Agent *Agent;
/**
 * Full addresses to AC functions in memory, by default are hardcoded to
 * relative addresses but will get dynamically changed by injector script.
 */
struct
{
    __uint64_t checkinput = 0x69b00;
    __uint64_t raycube = 0x79970;
    __uint64_t TraceLine = 0x12c8d0;
    __uint64_t _setskin = 0x2e5f0;
    __uint64_t calcteamscores = 0xb1930;
    __uint64_t mapmodelslotusage = 0x957b0;
    __uint64_t drawradarent = 0x8dfd0;
    __uint64_t particle_trail = NULL;
} AC_function_addresses;

/**
 * Full addresses to useful symbols in AC memory, not hardcoded by default and
 * are expected to be dynamically changed by injector script.
 */
struct
{
    __uint64_t player1;
    __uint64_t players;
    __uint64_t ents;
    __uint64_t screenw;
    __uint64_t screenh;
    __uint64_t mvpmatrix;
} AC_symbol_addresses;

/**
 * Stuff we want to use in our hook wherever.
 */
struct
{
    void *handle;
    Environment_Interaction *interface;
    Entity_Tracker *resolver;
    ConditionalAgent *agent;
    AC_detour detour;
} hook_util;

void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);

    // giving our boy infinite health + ammo
    features.player1->set_health(999);
    features.player1->set_rifle_ammo(99);

    Agent->Navigate();

    // aimbot
    for (dynamic_ent *ent: features.dynamic_entities) {
        if (ent->health <= 0) {
            // no shooting if we can't see enemy or they are off-screen
            hook_util.interface->Mouse_Button_Event(false);
            continue;
        }
        // seeing if enemy is on our screen
        float screenX, screenY = 0;
        WorldToScreen(ent->position, features.mvpmatrix, features.screenw, features.screenh, &screenX, &screenY);
        bool onScreen = (0 < screenX && screenX < features.screenw) && (0 < screenY && screenY < features.screenh);

        // seeing if they are actually visible
        float enemyDistance = GetVectorDistance(features.player1->position, ent->position);
        float traceDistance = GetVectorDistance(features.player1->position, ent->trace->end);
        bool isVisible = traceDistance - 2.0f < enemyDistance && enemyDistance < traceDistance + 2.0f;

        if (onScreen && isVisible) {
            vec angle = GetRayAngle(features.player1->position, ent->position);
            angle.x += 180;
            if (angle.x > 360) {
                angle.x -= 360;
            }
            features.player1->set_yaw_pitch(angle.x, angle.y);
            hook_util.interface->Mouse_Button_Event(true);
            break;
        }
        else {
            // no shooting if we can't see enemy or they are off-screen
            hook_util.interface->Mouse_Button_Event(false);
        }
    }
    outFile.close();
}

void __attribute__((naked)) trampoline_function()
{
    // executing stolen instructions
    __asm__(
        "lea 2(%%rip),%%rdx;"
        "jmp *%0;"
        :
        : "r" (hook_util.detour.original_instructions)
    );

    // calling hook function
    __uint64_t hook_function_address = (__uint64_t)&hook_function;
    __asm__(
        "call *%0;"
        :
        : "r" (hook_function_address)
    );

    // return control flow to the game
    __uint64_t return_address = (__uint64_t)hook_util.detour.hook_victim_address + AC_detour::hook_instruction_length;
    __asm__(
        "mov %0, %%rdx;"
        "jmp *%%rdx;"
        :
        : "r" (return_address)
        : "%rdx"
    );
}           

void init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);

    if (!outFile.is_open()) {
        outFile << "Error: Could not open the log file!" << std::endl;
        //exit(1);  // Exit or handle the error accordingly
    }
    // load some sdl library functions from inside ac
    hook_util.handle = dlopen("native_client", RTLD_LAZY | RTLD_NOLOAD);

    outFile << std::hex << hook_util.handle << std::endl;
    void *sdl_pushevent_address = dlsym(hook_util.handle, "SDL_PushEvent");
    void *sdl_getmousestate_address = dlsym(hook_util.handle, "SDL_GetMouseState");

    // establish the interaction api
    hook_util.interface = new Environment_Interaction;
    hook_util.interface->sdl_util.SDL_PushEvent = (int (*)(SDL_Event*))sdl_pushevent_address;
    hook_util.interface->sdl_util.SDL_GetMouseState = (__uint32_t (*)(int *x, int *y))sdl_getmousestate_address;

    // initiate hook
    hook_util.detour = AC_detour(AC_function_addresses.checkinput, (__uint64_t)&trampoline_function);
    outFile << "successfully hooked" << std::endl;

    // intiate the feature resolver and grab some functions from the game
    hook_util.resolver = new Entity_Tracker(AC_symbol_addresses.player1, AC_symbol_addresses.players, AC_symbol_addresses.ents, &features);
    features.screenw = *(__uint64_t *)AC_symbol_addresses.screenw;
    features.screenh = *(__uint64_t *)AC_symbol_addresses.screenh;
    features.mvpmatrix = (float *)AC_symbol_addresses.mvpmatrix;

    hook_util.resolver->TraceLine = (void (*)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr))AC_function_addresses.TraceLine;
    hook_util.resolver->patricle_trail = (void (*)(int type, int fade, vec s, vec e))AC_function_addresses.particle_trail;
    hook_util.resolver->drawradarent = (void (*)(float x, float y, float yaw, int col, int row, float iconsize, bool pulse, const char *label,...))AC_function_addresses.drawradarent;
    
    outFile << "enemies " << features.dynamic_entities.size() << std::endl;

    Agent = new Hunter_Agent(hook_util.resolver, hook_util.interface);

    outFile << "initiated hunter agent" << std::endl;

    outFile.close();
}

void __attribute__((destructor)) unload()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log", std::ios::app);

    // dlclose(hook_util.handle);
    mprotect((void*)hook_util.detour.executable_page_address, AC_detour::target_page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    std::memcpy((void *)((__uint64_t)hook_util.detour.hook_victim_address), (void *)hook_util.detour.original_instructions, AC_detour::hook_instruction_length);
    mprotect((void*)hook_util.detour.executable_page_address, AC_detour::target_page_size, PROT_READ | PROT_EXEC);

    if (munmap(hook_util.detour.original_instructions, AC_detour::hook_instruction_length + 2) == -1) {
        perror("munmap error");
        return;
    }

    outFile << "successfully unhooked\n";
    outFile.close();
}