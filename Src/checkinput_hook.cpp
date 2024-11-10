#include "ac_detour.h"
#include "Environment_Interaction.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cmath>
#include "Feature_Resolver.h"
#include <iostream>
#include "agents/conditional/conditional_agent.h"

SDL_keys sdl_keys;
Features features;

struct Data_Link
{
    Features *GameState; // placeholders
    __uint8_t *GameAction[64];
};

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
    Feature_Resolver *resolver;
    void (*patricle_trail)(int type, int fade, vec s, vec e);

    AC_detour detour;
} hook_util;

ConditionalAgent *agent;
bool agentSet = false;

// super evil code >:)
void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    if (!agentSet) {
        agent = new ConditionalAgent(hook_util.resolver->features, hook_util.interface);
    }

    hook_util.resolver->Resolve_Dynamic_Entities();
    hook_util.resolver->TNB_Ray_Trace();
    agent->run();

    outFile.close();
    return;

    // resolve the gamestate, write to shared memory
    //////////////// updating critical section
    std::cout << "resolving players" << std::endl;
    hook_util.resolver->Resolve_Dynamic_Entities();
    for (dynamic_ent *e : *features.dynamic_entities)
    {
        std::cout << e->x << e->y << e->z << std::endl;
        //outFile << e->x << e->y << e->z << std::endl;
    }

    outFile << "resolving ents" << std::endl;
    hook_util.resolver->Resolve_Static_Entities();

    for (static_ent *e : *features.static_entities)
    {
        float distance = sqrt(pow(features.player1->x-e->trace->end.x,2)+pow(features.player1->y-e->trace->end.y,2)+pow(features.player1->z-e->trace->end.z,2));
        outFile << e->trace->collided << " distance: " << distance << " " << e->type << std::endl;
    }

    hook_util.resolver->TNB_Ray_Trace();
    return;
    ///////////////// end update

    // outFile << "yaw: " << hook_util.resolver->features->player1->yaw << " pitch: " << hook_util.resolver->features->player1->pitch << std::endl;
    // outFile << "x: " << hook_util.resolver->features->player1->x << "y: " << hook_util.resolver->features->player1->y << "z: " << hook_util.resolver->features->player1->z << std::endl;

    // // calculate the distance of the rays
    // float distance = sqrt(pow(hook_util.resolver->features->player1->x-features.rays[0].end.x,2)+pow(hook_util.resolver->features->player1->y-features.rays[0].end.y,2)+pow(hook_util.resolver->features->player1->z+5.5-features.rays[0].end.z,2));
    // outFile << "forwards: " << distance << std::endl;
    // outFile << features.rays[0].collided << " " << features.rays[0].end.x << " " << features.rays[0].end.y << " " << features.rays[0].end.z << std::endl;
    
    // distance = sqrt(pow(hook_util.resolver->features->player1->x-features.rays[1].end.x,2)+pow(hook_util.resolver->features->player1->y-features.rays[1].end.y,2)+pow(hook_util.resolver->features->player1->z+5.5-features.rays[1].end.z,2));
    // outFile << "right: " << distance << std::endl;
    // outFile << features.rays[1].collided << " " << features.rays[1].end.x << " " << features.rays[1].end.y << " " << features.rays[1].end.z << std::endl;
    
    // distance = sqrt(pow(hook_util.resolver->features->player1->x-features.rays[2].end.x,2)+pow(hook_util.resolver->features->player1->y-features.rays[2].end.y,2)+pow(hook_util.resolver->features->player1->z+5.5-features.rays[2].end.z,2));
    // outFile << "back: " << distance << std::endl;
    // outFile << features.rays[2].collided << " " << features.rays[2].end.x << " " << features.rays[2].end.y << " " << features.rays[2].end.z << std::endl;
    
    // distance = sqrt(pow(hook_util.resolver->features->player1->x-features.rays[3].end.x,2)+pow(hook_util.resolver->features->player1->y-features.rays[3].end.y,2)+pow(hook_util.resolver->features->player1->z+5.5-features.rays[3].end.z,2));
    // outFile << "left: " << distance << std::endl;
    // outFile << features.rays[3].collided << " " << features.rays[3].end.x << " " << features.rays[3].end.y << " " << features.rays[3].end.z << std::endl;

    // distance = sqrt(pow(hook_util.resolver->features->player1->x-features.rays[4].end.x,2)+pow(hook_util.resolver->features->player1->y-features.rays[4].end.y,2)+pow(hook_util.resolver->features->player1->z+5.5-features.rays[4].end.z,2));
    // outFile << "up: " << distance << std::endl;
    // outFile << features.rays[4].collided << " " << features.rays[4].end.x << " " << features.rays[4].end.y << " " << features.rays[4].end.z << std::endl;
    
    // distance = sqrt(pow(hook_util.resolver->features->player1->x-features.rays[5].end.x,2)+pow(hook_util.resolver->features->player1->y-features.rays[5].end.y,2)+pow(hook_util.resolver->features->player1->z+5.5-features.rays[5].end.z,2));
    // outFile << "down: " << distance << std::endl;
    // outFile << features.rays[5].collided << " " << features.rays[5].end.x << " " << features.rays[5].end.y << " " << features.rays[5].end.z << std::endl;

    // wait for response
    // read actions from shared memory

    // manipulate event queue

    // int result = hook_util.interface->Mouse_Button_Event();

    // outFile << result << std::endl;
    // if (result != 1) {
    //     outFile << "Failed to push event " << std::endl;
    // } else {
    //     outFile << "Event pushed successfully." << std::endl;
    // }

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

void /*__attribute__((constructor))*/ init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

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

    // locate player1
    /*__uint32_t player_ip_offset = *(__uint32_t*)(AC_function_addresses._setskin + 0x6);
    __uint64_t player_base_address = *(__uint64_t*)(AC_function_addresses._setskin + 0xa + player_ip_offset);
    
    // locate player_ents
    __uint32_t players_ip_offset = *(__uint32_t*)(AC_function_addresses.calcteamscores + 0x40);
    __uint64_t players_base_address = AC_function_addresses.calcteamscores + 0x44 + players_ip_offset;

    // locate other ents
    __uint32_t ents_ip_offset = *(__uint32_t*)(AC_function_addresses.mapmodelslotusage + 0x3a);
    __uint64_t ents_base_address = AC_function_addresses.mapmodelslotusage + 0x3e + ents_ip_offset;*/

    // establish the feature resolver and grab some functions from the game
    hook_util.resolver = new Feature_Resolver(AC_symbol_addresses.player1, AC_symbol_addresses.players, AC_symbol_addresses.ents, &features);
    hook_util.resolver->features->screenw = *(__uint64_t *)AC_symbol_addresses.screenw;
    hook_util.resolver->features->screenh = *(__uint64_t *)AC_symbol_addresses.screenh;
    hook_util.resolver->features->mvpmatrix = (float *)AC_symbol_addresses.mvpmatrix;

    outFile << AC_function_addresses.TraceLine << std::endl;
    hook_util.resolver->TraceLine = (void (*)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr))AC_function_addresses.TraceLine;

    outFile.close();
}

void __attribute__((destructor)) unload()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

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
