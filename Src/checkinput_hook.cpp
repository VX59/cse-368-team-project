#include "ac_detour.h"
#include "Environment_Interaction.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cmath>
#include "Feature_Resolver.h"
#include <iostream>
#include <random>
#include <algorithm>
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
    Feature_Resolver *resolver;

    AC_detour detour;
} hook_util;

ConditionalAgent *agent;
bool agentSet = false;

float weight(float x, float rho, float offset)
{
    return (1/(rho*std::sqrt(2*M_PI)))*std::exp(pow((-1/2)*((x-offset)/rho),2));
}

// super evil code >:)

void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    if (!agentSet) {
        agent = new ConditionalAgent(hook_util.resolver->features, hook_util.interface);
    }

    hook_util.resolver->Resolve_Dynamic_Entities();

    // given the objective node see if a path in the graph exists to it
    // if it does calculate the path and proceed
    // otherwise turn on exploration mode

    // node objectives is the path the agent is following. On starting and every time we reach an objective we should
    // evaluate the objectives.

    // assuming the first item in the objectives is the objectives is the actual target and everything stacked are waypoints
    // the objectives list cannot be emptied until the current node and the end of the target are
    // BOTH in the connected nodes list.

    // Once the objectives are emptied then we can choose a new target

    // first check if the distance to a node is low enough to warrant objective eval

    float obj_dist = std::sqrt(pow(hook_util.resolver->features->node_positions[hook_util.resolver->features->objective_nodes.back()].x-hook_util.resolver->features->player1->x,2)+
                            pow(hook_util.resolver->features->node_positions[hook_util.resolver->features->objective_nodes.back()].y-hook_util.resolver->features->player1->y,2));
    if (obj_dist < 1)
    {
        hook_util.resolver->features->current_node = hook_util.resolver->features->objective_nodes.back();
        hook_util.resolver->features->objective_nodes.pop_back();

        // after reaching an objective and on start we see if there are more objectives
        // if not make a random one
        if (hook_util.resolver->features->objective_nodes.size() == 0)
        {
            std::random_device rd;
            std::mt19937 gen(rd());

            std::uniform_real_distribution<int> dist(0, hook_util.resolver->features->nodes);

            // push a random objective point
            hook_util.resolver->features->objective_nodes.push_back(dist(gen));
        }

        auto found = std::find(hook_util.resolver->features->connected_nodes.begin(),hook_util.resolver->features->connected_nodes.end(), 
            hook_util.resolver->features->objective_nodes.back());

        if (found != hook_util.resolver->features->connected_nodes.end())
        {
            // check if objectives is a path from current to target node .. if not its probably just 1 random point
            // if objectives is not a path .. add the path to it using djikstra or astar

            // if objectives is a path we dont have to do anything just follow it
        } else {
            // if its not connected explore, find k closest, try the furthest non colliding
            int k=4;
            std::vector<int> indices(hook_util.resolver->features->nodes);
            std::vector<float> distances(hook_util.resolver->features->nodes);

            float cx = hook_util.resolver->features->player1->x;
            float cy = hook_util.resolver->features->player1->y;

            std::iota(indices.begin(), indices.end(), 0);
            std::transform(hook_util.resolver->features->node_positions.begin(),
                            hook_util.resolver->features->node_positions.end(),
                            distances.begin(), [cx,cy](vec v) { return std::sqrt(pow(v.x-cx,2)+pow(v.y-cy,2));});

            // Sort the indices based on the corresponding values in vec
            std::partial_sort(indices.begin(), indices.begin() + k, indices.end(),
                            [&distances](int i1, int i2) { return distances[i1] < distances[i2]; });

            // Return the closest k nodes
            std::vector<int> min_nodes(indices.begin(), indices.begin() + k);

            // draw rays
            std::vector<int> no_collide_neighbors;
            for (int i : min_nodes)
            {
                vec node = hook_util.resolver->features->node_positions[i];
                traceresult_s tr;
                hook_util.resolver->Target_Ray_Trace(node, &tr);

                // if no collision add it to the graph
                if (!tr.collided)
                {
                    // fix later .. check if its already there
                    hook_util.resolver->features->node_adjacency_mat[i][hook_util.resolver->features->current_node] = 1;
                    hook_util.resolver->features->node_adjacency_mat[hook_util.resolver->features->current_node][i] = 1;
                    hook_util.resolver->features->connected_nodes.push_back(i);
                    no_collide_neighbors.push_back(i);
                }
            }

            // set a objective to the furthest non colliding k-node
            // stacks in objectives to next tick we see this node in the graph and move to it
            hook_util.resolver->features->objective_nodes.push_back(no_collide_neighbors.back());
        }
    }

    // turn towards the next objective
    ///...
    // move forwards
    ///...
    
    outFile << hook_util.resolver->features->player1->x << " " << hook_util.resolver->features->player1->y;

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

    // establish the feature resolver and grab some functions from the game
    hook_util.resolver = new Feature_Resolver(AC_symbol_addresses.player1, AC_symbol_addresses.players, AC_symbol_addresses.ents, &features);
    hook_util.resolver->features->screenw = *(__uint64_t *)AC_symbol_addresses.screenw;
    hook_util.resolver->features->screenh = *(__uint64_t *)AC_symbol_addresses.screenh;
    hook_util.resolver->features->mvpmatrix = (float *)AC_symbol_addresses.mvpmatrix;

    hook_util.resolver->TraceLine = (void (*)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr))AC_function_addresses.TraceLine;
    hook_util.resolver->patricle_trail = (void (*)(int type, int fade, vec s, vec e))AC_function_addresses.particle_trail;

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
