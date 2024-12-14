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

    int current;
    while (!Queue.empty())
    {
        current = Queue.front();
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

    if (current != T)
    {
        return -1;
    }

    // recover path
    int i = 0;
    for (int node = T; node != -1; node = Parent[node])
    {
        if (node < features.nodes && node != -1)
        {
            features.objective_nodes.push_back(node);
            i++;
        }
    }
    features.objective_nodes.pop_back();
    features.objective_is_path = true;
    return i;
}

// super evil code >:)

float flat_distance(vec v1, vec v2) {
    return sqrt(pow(v2.x - v1.x, 2) + pow(v2.y - v1.y, 2));
}

void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log", std::ios::app);

    // giving our boy infinite health + ammo
    features.player1->set_health(999);
    features.player1->set_rifle_ammo(99);

    float old_obj_dist, obj_dist, obj_velocity;
    hook_util.resolver->Resolve_Dynamic_Entities();

    if (!features.objective_nodes.empty())
    {
        old_obj_dist = flat_distance(features.node_positions[features.objective_nodes.back()],features.player1->position);
        obj_dist = flat_distance(features.node_positions[features.objective_nodes.back()],features.player1->position);
    } else 
    {
        outFile << "objectives is empty" << std::endl;
        old_obj_dist = 0;
        obj_dist = 0;
    }

    obj_velocity = old_obj_dist - obj_dist;
    outFile << "objective velocity " << fabs(obj_velocity) << " objective dist " << obj_dist << std::endl;

    int prox = 2;

    if (obj_dist < prox)
    {   
        // This causes the agent to spasm, but stops a segfault from popping
        // on an empty objective nodes list
        //
        // THIS IS WHAT IS CAUSING OCCASIONAL CRASHES (WHEN THERE IS NO IF CHECK)
        if (!features.objective_nodes.empty()) {
            features.current_node = features.objective_nodes.back();
            features.objective_nodes.pop_back();
        }

        outFile << "Exploring the map" << std::endl;

        int k=sqrtf(features.nodes);

        std::vector<float> distances;

        for (int i = 0; i < features.connected_pool.size(); i++)
        {
            if (i == features.current_node)
            {
                distances.push_back(1e7);
                continue;
            }
            distances.push_back(flat_distance(features.node_positions[features.current_node], features.node_positions[i]));
        }
        
        outFile << "sorting nodes" << std::endl;

        std::vector<int> indices(features.connected_pool.size());

        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&distances](float i1, float i2) {
            return distances[i1] < distances[i2];
        });

        //Return the closest k nodes

        std::vector<int> min_nodes(indices.begin(), indices.begin() + k);

        for (int m : min_nodes)
        {
            outFile << "m " << m << std::endl;
        }

        // draw rays to closest k nodes count how many are visible
        for (int i = 0; i < k; i++)
        {
            if (min_nodes[i] < features.nodes && min_nodes[i] != features.current_node)
            {
                vec node = features.node_positions[min_nodes[i]];
                if (node.x == -1) continue;
                traceresult_s tr;
                hook_util.resolver->Target_Ray_Trace(node, &tr);

                if (!tr.collided)
                {
                    if (features.connected_pool[min_nodes[i]] == 0)
                    {
                        features.node_adjacency_mat[min_nodes[i]][features.current_node] = 1;
                        features.node_adjacency_mat[features.current_node][min_nodes[i]] = 1;
                    }
                }
            }
        }
 
        // if we couldnt find any visible nodes draw a ray in a random yaw and go halfway
        // if there are unallocated nodes we can add that point to the graph
        if (features.objective_nodes.empty())
        {
            outFile << "scanning environment " << std::endl;
            vec from = features.player1->position;
            from.z += 5.5;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> ryaw(0,360);

            vec to;
            
            for (int i = 0; i < k; i++)
            {
                if (features.free_nodes == 0) break;

                float scan_distance = 0;

                while (scan_distance < pow(prox,2))
                {
                    double yaw = (ryaw(gen)) * (M_PI/180.f);
                    double pitch = 0;
                    float epsilon = 0.15;
                    // calculate 100 cube ray
                    float limit = 1000.f;

                    to.x = from.x + (cos(yaw) * cos(pitch)) * limit;
                    to.y = from.y + (sin(yaw) * cos(pitch)) * limit;
                    to.z = from.z + sin(pitch) * limit;

                    traceresult_s tr;
                    
                    hook_util.resolver->Target_Ray_Trace(to, &tr);

                    vec delta = {(tr.end.x-from.x)/2, (tr.end.y-from.y)/2, from.z};
                    scan_distance = sqrtf(pow(delta.x,2) + pow(delta.y,2));

                    to = {from.x + delta.x, from.y + delta.y, from.z};
                }

                auto it = std::find(features.free_pool.begin(), features.free_pool.end(), 1);
                int idx = std::distance(features.free_pool.begin(), it);

                hook_util.resolver->Add_Node(to, idx);
                features.node_adjacency_mat[idx][features.current_node] = 1;
                features.node_adjacency_mat[features.current_node][idx] = 1;
            }
        }
    }   
    vec default_pos = {1e7,1e7,1e7};
    // only happens when were stuck away from a node
    if (obj_velocity == 0 && obj_dist >= prox && !features.objective_nodes.empty())
    {
        outFile << "evaluating objective velocity " << std::endl;
        outFile << "cleared objectives .. proceed to find a new path" << std::endl;

        int node = features.objective_nodes.back();
        features.objective_nodes.clear();

        if (node == features.current_node)
        {
            auto it = std::find(features.free_pool.begin(), features.free_pool.end(), 1);
            int idx = std::distance(features.free_pool.begin(), it);

            hook_util.resolver->Add_Node(features.player1->position, idx);
            features.current_node = idx;
            outFile << "adding a new node " << idx << std::endl;

        }
        else
        {
            outFile << "PRUNING NODE " << node << " current node " << features.current_node << std::endl;
            vec default_pos = {1e7,1e7,1e7};

            // unadd the node
            for (int i = 0; i < features.node_adjacency_mat.size(); i++)
            {
                features.node_adjacency_mat[node][i] = 0;
                features.node_adjacency_mat[i][node] = 0;
            }

            features.node_positions[node] = default_pos;
            features.connected_pool[node] = 0;
            features.free_pool[node] = 1;
            features.nodes --;
            features.free_nodes ++;     
        }
    }   

    if (!features.objective_nodes.empty())
    {
        outFile << features.objective_nodes.size() << " OBJECTIVES" << std::endl;
        for (int o : features.objective_nodes)
        {
            vec pos = features.node_positions[o];
            outFile << o << " x: " << pos.x << " y: " << pos.y << "z: " << pos.z << std::endl;
        }
    }

    if (features.objective_nodes.empty())
    {
        hook_util.interface->Keyboard_Event(sdl_keys.SDLK_w, false);

        outFile << "selecting a path" << std::endl;

        std::vector<int> connected_nodes;
        for (int i = 0; i < features.nodes; i++)
        {
            if (features.connected_pool[i] > 0 && i != features.current_node)
            {
                connected_nodes.push_back(i);
            }
        }
        if (connected_nodes.empty())
        {
            outFile << "no connected nodes" << std::endl;
            return;
        }

        outFile << "current node " << features.current_node << std::endl;

        vec cur_pos, nex_pos;
        int best_pick;

        outFile << "target position " << features.target.x << " " << features.target.y << std::endl;
        if(features.target.x != default_pos.x)
        {
            outFile << "estimating path to target" << std::endl;

            std::vector<float> distances;
            for (int i = 0; i < features.connected_pool.size(); i++)
            {
                if (i == features.current_node || features.connected_pool[i] == 0)
                {
                    distances.push_back(1e7);
                } 
                else
                {
                    distances.push_back(flat_distance(features.target, features.node_positions[i]));
                }
            }
            std::vector<int> indices(distances.size());

            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(), [&distances](float i1, float i2) {
                return distances[i1] < distances[i2];
            });


            best_pick = *indices.begin();
            outFile << "closest distance " << distances[best_pick] << std::endl;
            if(distances[best_pick] > 1e6)
            {
                outFile << "undiscovered" << std::endl;
                return;
            }

            outFile << "next node is " << best_pick << std::endl;
        } else
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, connected_nodes.size()-1);
            best_pick = dist(gen);
        }

        if (best_pick != features.current_node)
        {
            cur_pos = features.node_positions[features.current_node];
            nex_pos = features.node_positions[best_pick];

            outFile << "BFSing from " << features.current_node << " " << cur_pos.x << " " << cur_pos.y << " to " << best_pick << " " << nex_pos.x << " " << nex_pos.y << std::endl;
            int i = BFS(features.current_node, best_pick);
            outFile << "path length " << i << std::endl;
        }
    }

    if (!features.objective_nodes.empty())
    {
        vec player_pos = {features.player1->position.x, features.player1->position.y, features.player1->position.z};
        vec target = features.node_positions[features.objective_nodes.back()];
        target.z = features.player1->position.z;
        vec angular_displacement = GetRayAngle(player_pos,target);
        angular_displacement.x += 180;
        if (angular_displacement.x > 360) {
            angular_displacement.x -= 360;
        }
        features.player1->set_yaw_pitch(angular_displacement.x, angular_displacement.y);

        hook_util.interface->Keyboard_Event(sdl_keys.SDLK_w, true);

        outFile << "P1 pos x: "<< features.current_node << " " << features.player1->position.x << " y: " << features.player1->position.y << "z: " << features.player1->position.z << std::endl;   
    }
    // outFile << "graph explored " << connected <<" / " << features.nodes << " : %" << graph_explored << std::endl;

    // aimbot
    for (dynamic_ent *ent: features.dynamic_entities) {
        // if player is on our team or dead don't aim at them
        if (ent->team == features.player1->team || ent->health <= 0) {
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

    // if (hook_util.handle == nullptr)
    // { 
    //     outFile << "couldn't open the application" << std::endl;
    //     exit(1);
    // }

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

    // initiate the agent

    // intiate the feature resolver and grab some functions from the game
    hook_util.resolver = new Entity_Tracker(AC_symbol_addresses.player1, AC_symbol_addresses.players, AC_symbol_addresses.ents, &features);
    features.screenw = *(__uint64_t *)AC_symbol_addresses.screenw;
    features.screenh = *(__uint64_t *)AC_symbol_addresses.screenh;
    features.mvpmatrix = (float *)AC_symbol_addresses.mvpmatrix;

    hook_util.resolver->TraceLine = (void (*)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr))AC_function_addresses.TraceLine;
    hook_util.resolver->patricle_trail = (void (*)(int type, int fade, vec s, vec e))AC_function_addresses.particle_trail;
    hook_util.resolver->drawradarent = (void (*)(float x, float y, float yaw, int col, int row, float iconsize, bool pulse, const char *label,...))AC_function_addresses.drawradarent;
    
    outFile << "enemies " << features.dynamic_entities.size() << std::endl;
    outFile << "initiating navigation" << std::endl;
    if(!features.dynamic_entities.empty())
    {
        outFile << "searching for a target" << std::endl;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0,features.dynamic_entities.size()-1);
        int rndx = dist(gen);
        dynamic_ent target_ent = *(features.dynamic_entities[rndx]);

        features.target = target_ent.position;
        outFile.close();
    } else
    {
        outFile << "no entities found to target .. proceeding to explore" << std::endl;
        vec default_pos = {1e7,1e7,1e7};
        features.target = default_pos;
    }

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
