#include "ac_detour.h"
#include "Environment_Interaction.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cmath>
#include "Feature_Resolver.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include "agents/conditional/conditional_agent.h"
#include "./agents/math_helpers.h"

SDL_keys sdl_keys;
Features features;

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

// start with bfs
int BFS(int S, int T)
{
    std::vector<std::vector<int>> Graph = features.node_adjacency_mat;
    std::unordered_map<int,int> Parent;
    std::queue<int> Queue;
    std::vector<bool> Discovered(Graph.front().size(),false);

    Queue.push(S);
    Discovered[S] = true;
    Parent[S] = -1;
    while (!Queue.empty())
    {
        int current = Queue.front();
        Queue.pop();

        if (current == T)
        {
            break;
        }

        for (long unsigned int i = 0; i < features.node_adjacency_mat[current].size(); i++)
        {            
            int n = features.node_adjacency_mat[current][i] | features.node_adjacency_mat[i][current];            
            if (n > 0 && !Discovered[i])
            {
                Discovered[i] = true;
                Queue.push(i);
                Parent[i] = current;
            }
        }
    }

    // recover path
    int i = 0;
    for (int node = T; node != -1; node = Parent[node])
    {
        if (node == Parent[node])
        {
            return -1;
        }
        if (node < features.nodes && node != -1)
        {
            features.objective_nodes.push_back(node);
            i++;
        } else
        {
            return -1;
        }
    }

    features.objective_is_path = true;
    return i;
}

// super evil code >:)

void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    float oldx = features.player1->x;
    float oldy = features.player1->y;
    float old_obj_dist = std::sqrt(pow(features.node_positions[features.objective_nodes.back()].x-oldx,2)+
                                pow(features.node_positions[features.objective_nodes.back()].y-oldy,2));

    hook_util.resolver->Resolve_Dynamic_Entities();

    float obj_dist = std::sqrt(pow(features.node_positions[features.objective_nodes.back()].x-features.player1->x,2)+
                                pow(features.node_positions[features.objective_nodes.back()].y-features.player1->y,2));
    
    float obj_velocity = old_obj_dist - obj_dist;
    
    if (obj_dist < 2)
    {
        features.current_node = features.objective_nodes.back();
        features.objective_nodes.pop_back();
        
        outFile << "Exploring the map" << std::endl;
        int k=16;
        std::vector<int> indices(features.nodes);
        std::vector<float> distances(features.nodes);

        float cx = features.node_positions[features.current_node].x;
        float cy = features.node_positions[features.current_node].y;

        for (int i=0; i < features.nodes; i++)
        {
            distances[i] = std::sqrt(pow(cx-features.node_positions[i].x,2) + pow(cy-features.node_positions[i].y,2));
        }

        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&distances](float i1, float i2) {
            return distances[i1] < distances[i2];
        });

        //Return the closest k nodes
        std::vector<int> min_nodes(indices.begin(), indices.begin() + k);

    // draw rays            
        for (int i = 0; i < k; i++)
        {
            if (min_nodes[i] < features.nodes)
            {
                vec node = features.node_positions[min_nodes[i]];
                traceresult_s tr;
                hook_util.resolver->Target_Ray_Trace(node, &tr);


            // if no collision add it to the graph
                outFile << min_nodes[i] << " collided : " << tr.collided << " explored : " << features.connected_nodes[min_nodes[i]] << std::endl;
                if (!tr.collided && features.connected_nodes[min_nodes[i]] == 0)
                {
   
                    features.node_adjacency_mat[min_nodes[i]][features.current_node] = 1;
                    features.node_adjacency_mat[features.current_node][min_nodes[i]] = 1;
                    features.connected_nodes[min_nodes[i]] = 1;
                    features.discovery_nodes.push_back(min_nodes[i]);
                    
                    outFile << "adding " << min_nodes[i] << " to no discovery nodes" << std::endl;
                }
            }
        }
    }

    if (obj_velocity == 0)
    {
        outFile << "evaluating objective velocity " << obj_velocity << std::endl;
        features.node_adjacency_mat[features.current_node][features.objective_nodes.back()] = -1;
        features.node_adjacency_mat[features.objective_nodes.back()][features.current_node] = -1;
        features.connected_nodes[features.objective_nodes.back()] = 0;
        
        features.objective_nodes.clear();
        
        int k = 8;
        int std = 8;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> xdist(features.player1->x, std);
        std::normal_distribution<float> ydist(features.player1->y, std);
        std::uniform_real_distribution<float> zdist(features.player1->z,features.player1->z+6);
    
        for (int i = 0; i < k; i ++)
        {
            vec node = {xdist(gen), ydist(gen), zdist(gen)};
            features.node_positions.push_back(node);
        }

        features.connected_nodes.resize(features.nodes + k,0);
        features.nodes += k;
        features.node_adjacency_mat.resize(features.nodes+k,std::vector<int>(features.nodes,0));
        
        for(int i = 0; i < features.nodes; i++)
        {
            features.node_adjacency_mat[i].resize(features.nodes+k,0);
        }
    }   

    if (features.objective_nodes.empty())
    {
        // pick a random connected node and bfs to it.. or least visited
        outFile << "selecting a path" << std::endl;

        std::random_device rd;
        std::mt19937 gen(rd());

        if (!features.discovery_nodes.empty())
        {
            outFile << "searching discovery nodes" << std::endl;
            std::uniform_int_distribution<> dist(0, features.discovery_nodes.size());

            int rndindx = dist(gen);
            BFS(features.current_node, features.discovery_nodes[rndindx]);

            features.discovery_nodes.erase(features.discovery_nodes.begin()+rndindx);
            outFile << "found a path to the objective" << std::endl;
            
        } else {
            std::vector<int> connected_nodes;

            for (int i = 0; i < features.nodes; i++)
            {
                if (features.connected_nodes[i] > 0)
                {
                    connected_nodes.push_back(i);
                }
            }
            outFile << "connected nodes " << connected_nodes.size() << std::endl;
            if (!connected_nodes.empty())
            {
                std::uniform_int_distribution<> dist(0, connected_nodes.size());
                BFS(features.current_node, connected_nodes[dist(gen)]);
            }
        }

    }

    if (!features.objective_nodes.empty())
    {
        vec player_pos = {features.player1->x, features.player1->y, features.player1->z};
        vec target = features.node_positions[features.objective_nodes.back()];
        target.z = features.player1->z;
        vec angular_displacement = GetRayAngle(player_pos,target);
        angular_displacement.x += 180;
        if (angular_displacement.x + 360) {
            angular_displacement.x -= 360;
        }
        features.player1->set_yaw_pitch(angular_displacement.x, angular_displacement.y);

        hook_util.interface->Keyboard_Event(sdl_keys.SDLK_w, hook_util.interface->sdl_util.SDL_KEYDOWN,1);
    }

    int connected = 0;
    for(int c : features.connected_nodes)
    {
        if (c > 0)
        {
            connected ++;
        }
    }
    float graph_explored = (float)(connected)/(float)(features.random_nodes);
    if (features.objective_nodes.empty())
    {
        outFile << "no objective !! or fully mapped path";
    }

    outFile << features.discovery_nodes.size() << " DISCOVERY NODES" << std::endl;
    for (int o : features.discovery_nodes)
    {
        vec pos = features.node_positions[o];
        outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
    }

    outFile << features.objective_nodes.size() << " OBJECTIVES" << std::endl;
    for (int o : features.objective_nodes)
    {
        vec pos = features.node_positions[o];
        outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
    }
    outFile << "graph explored " << connected <<" / " << features.nodes << " : %" << graph_explored << std::endl;
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
        std::cerr << "Error: Could not open the log file!" << std::endl;
        exit(1);  // Exit or handle the error accordingly
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

    // establish the feature resolver and grab some functions from the game
    hook_util.resolver = new Feature_Resolver(AC_symbol_addresses.player1, AC_symbol_addresses.players, AC_symbol_addresses.ents, &features);
    features.screenw = *(__uint64_t *)AC_symbol_addresses.screenw;
    features.screenh = *(__uint64_t *)AC_symbol_addresses.screenh;
    features.mvpmatrix = (float *)AC_symbol_addresses.mvpmatrix;

    hook_util.resolver->TraceLine = (void (*)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr))AC_function_addresses.TraceLine;
    hook_util.resolver->patricle_trail = (void (*)(int type, int fade, vec s, vec e))AC_function_addresses.particle_trail;

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
