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
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");


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
        outFile << "currrent " << current << std::endl;
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
                outFile << "found node " << i << " from " << current << std::endl;
                Discovered[i] = true;
                Queue.push(i);
                Parent[i] = current;
            }
        }
    }

    // recover path
    int i = 0;
    outFile << "recovering path " << Parent.size() << std::endl;
    for (int node = T; node != -1; node = Parent[node])
    {
        if (node == Parent[node])
        {
            return -1;
        }
        if (node < features.nodes && node != -1)
        {
            outFile << node << " <- " << Parent[node] << std::endl;
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

void new_random_objective()
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist(0, features.nodes);

    // push a random objective point
    features.objective_nodes.push_back(dist(gen));
    features.objective_is_path = false;
}

void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    if (!agentSet) {
        agent = new ConditionalAgent(hook_util.resolver->features, hook_util.interface);
    }

    hook_util.resolver->Resolve_Dynamic_Entities();

    // given the objective node see if a path in the graph exists to it
    // if it does calculate the path and proceed
    // otherwise turn on exploration mode

    // node objectives is the path the agent is followin g. On starting and every time we reach an objective we should
    // evaluate the objectives.

    // assuming the first item in the objectives is the objectives is the actual target and everything stacked are waypoints
    // the objectives list cannot be emptied until the current node and the end of the target are
    // BOTH in the connected nodes list.

    // Once the objectives are emptied then we can choose a new target

    // first check if the distance to a node is low enough to warrant objective eval

    // turn towards the next objective
    // calculate theta from the position of the objective
    // xy distance

    if (features.objective_nodes.empty())
    {
        // pick a random connected node and bfs to it, set following path to true
        outFile << "selecting a path" << std::endl;

        std::vector<int> connected_nodes;

        for (int i = 0; i < features.nodes; i++)
        {
            if (features.connected_nodes[i] > 0)
            {
                connected_nodes.push_back(i);
            }
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, connected_nodes.size());
        int rndindx = dist(gen);

        int new_target = connected_nodes[rndindx];

        BFS(features.current_node, new_target);
    } else {
        float obj_dist = std::sqrt(pow(features.node_positions[features.objective_nodes.back()].x-features.player1->x,2)+
                                    pow(features.node_positions[features.objective_nodes.back()].y-features.player1->y,2));
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
            std::vector<int> no_collide_neighbors;
            
            for (int i = 0; i < k; i++)
            {
                if (min_nodes[i] < features.nodes)
                {
                    vec node = features.node_positions[min_nodes[i]];
                    traceresult_s tr;
                    hook_util.resolver->Target_Ray_Trace(node, &tr);


                // if no collision add it to the graph
                    outFile << min_nodes[i] << " collided : " << tr.collided << " explored : " << features.connected_nodes[min_nodes[i]] << std::endl;
                    if (!tr.collided)
                    {
                        outFile << "adding " << min_nodes[i] << " to no collide neighbors" << std::endl;
                        features.node_adjacency_mat[min_nodes[i]][features.current_node] = 1;
                        features.node_adjacency_mat[features.current_node][min_nodes[i]] = 1;
                        
                        if (features.connected_nodes[min_nodes[i]] == 0)
                        {
                            no_collide_neighbors.push_back(min_nodes[i]);
                            features.connected_nodes[min_nodes[i]] = 1;
                        }   

                    }
                }
            }

            if (no_collide_neighbors.size() > 0)
            {
                outFile << "selecting a visible neighbor" << std::endl;
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(0, no_collide_neighbors.size());
                int rndindx = dist(gen);
                if (rndindx >= 0 && rndindx < no_collide_neighbors.size())
                {
                    outFile << no_collide_neighbors.size() << " " << rndindx << std::endl;
                    features.objective_nodes.push_back(no_collide_neighbors[rndindx]);
                }
            }

            outFile << "added objectives" << std::endl;
            float obj_dist = std::sqrt(pow(features.node_positions[features.objective_nodes.back()].x-features.player1->x,2)+
                                            pow(features.node_positions[features.objective_nodes.back()].y-features.player1->y,2));
            outFile << features.objective_nodes.back() << " " << obj_dist << std::endl;
        }   
    }

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
    for (int o : features.objective_nodes)
    {
        vec pos = features.node_positions[o];
        outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
    }
    outFile << "graph explored " << connected <<" / " << features.random_nodes << " : %" << graph_explored << std::endl;

    // if (obj_dist < 3)
    // {

        // // after reaching an objective and on start we see if there are more objectives
        // // if not make a random one
        // if (features.objective_nodes.size() == 0) new_random_objective();

        // int found = features.connected_nodes[features.objective_nodes.back()];
        
        // outFile << features.objective_nodes.back() << " found : " << found << std::endl;
        
        // if (found > 0)
        // {
        //     outFile << "objective to target "<< features.objective_nodes.back() << " is a path : " << features.objective_is_path << std::endl;
        //     for (int o : features.objective_nodes)
        //     {
        //         vec pos = features.node_positions[o];
        //         outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
        //     } 
        //     if (!features.objective_is_path)
        //     {
        //         //pathfind
        //         BFS(features.current_node, features.objective_nodes.back());
                
        //         outFile << "path to target node added" << std::endl;
        //         for (int o : features.objective_nodes)
        //         {
        //             vec pos = features.node_positions[o];
        //             outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
        //         }                
        //     }
        // } else {
        //     // if the objective is not connected then we need to find the closest connected node
        //     int min_node = features.current_node;
        //     float min_dist = 1000;
        //     int i = 0;
        //     for (int n : features.connected_nodes)
        //     {
        //         if (n > 0)
        //         {
        //             float dist = std::sqrt(pow(features.node_positions[features.objective_nodes.back()].x-features.node_positions[i].x,2)
        //             + pow(features.node_positions[features.objective_nodes.back()].y-features.node_positions[i].y,2));
        //             if (dist < min_dist)
        //             {
        //                 min_node = i;
        //                 min_dist = dist;
        //             }
        //         }
        //         i++;
        //     }
        //     outFile << "closest connected node to " << features.objective_nodes.back() << " : " << min_node << " " << min_dist << std::endl;
        //     outFile << "min node " << min_node << " current node " << features.current_node << std::endl;
        //     // we are not at the edge of the known graph

        //     features.objective_nodes.push_back(min_node);
        //     outFile << "objective to min_node is a path : " << features.objective_is_path << std::endl;
        //     for (int o : features.objective_nodes)
        //     {
        //         vec pos = features.node_positions[o];
        //         outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
        //     } 

        //     if (min_node != features.current_node)
        //     {
        //         // is the objectives already a path to min node?
                
        //         if (!features.objective_is_path)
        //         {
        //             //pathfind
        //             // add the path to min_node
        //             outFile << "BFSing the path" << std::endl;

        //             BFS(features.current_node, min_node);
                    
        //             outFile << "path to closest connected node added" << std::endl;
        //             for (int o : features.objective_nodes)
        //             {
        //                 vec pos = features.node_positions[o];
        //                 outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
        //             }
        //         }

        //     } else {
        //         // only explore nodes when the next objective is not connected
        //         // if its not connected explore, find k closest, try the furthest non colliding
        //         outFile << "Exploring the map" << std::endl;
        //         int k=4;
        //         std::vector<int> indices(features.nodes);
        //         std::vector<float> distances(features.nodes);

        //         float cx = features.player1->x;
        //         float cy = features.player1->y;

        //         std::iota(indices.begin(), indices.end(), 0);
        //         std::transform(features.node_positions.begin(),
        //                         features.node_positions.end(),
        //                         distances.begin(), [cx,cy](vec v) { return std::sqrt(pow(v.x-cx,2)+pow(v.y-cy,2));});

        //         // Sort the indices based on the corresponding values in vec
        //         std::partial_sort(indices.begin(), indices.begin() + k, indices.end(),
        //                         [&distances](int i1, int i2) { return distances[i1] < distances[i2]; });

        //         // Return the closest k nodes
        //         std::vector<int> min_nodes(indices.begin(), indices.begin() + k);

        //         // draw rays
        //         std::vector<int> no_collide_neighbors;
                
        //         for (int i : min_nodes)
        //         {
        //             vec node = features.node_positions[i];
        //             traceresult_s tr;
        //             hook_util.resolver->Target_Ray_Trace(node, &tr);

        //             // if no collision add it to the graph
        //             //outFile << i << " collided : " << tr.collided << " explored : " << features.connected_nodes[i] << std::endl;
        //             if (!tr.collided)
        //             {
        //                 // check if its already in the graph
        //                 //outFile << " " << features.connected_nodes[i] << std::endl;
        //                 if (i < features.nodes)
        //                 {
        //                     //outFile << "adding " << i << " to no collide neighbors" << std::endl;
        //                     no_collide_neighbors.push_back(i);

        //                 } else {
        //                     outFile << "invalid node " <<std::endl;
        //                 }
        //             }
        //         }

        //         // set a objective to the furthest non colliding k-node
        //         // stacks in objectives to next tick we see this node in the graph and move to it
        //         // initiate exploration graph
        //         if (no_collide_neighbors.size() > 0)
        //         {
        //             outFile << "selecting a visible neighbor" << std::endl;
        //             std::random_device rd;
        //             std::mt19937 gen(rd());
        //             std::uniform_int_distribution<> dist(0, no_collide_neighbors.size()-1);
        //             int rndindx = dist(gen);
        //             outFile << no_collide_neighbors.size() << " " << rndindx << std::endl;
        //             features.objective_nodes.push_back(no_collide_neighbors[rndindx]);

        //             features.node_adjacency_mat[rndindx][features.current_node] = 1;
        //             features.node_adjacency_mat[features.current_node][rndindx] = 1;
        //             features.connected_nodes[rndindx] += 1;
                    
        //             features.objective_is_path = true;
        //         } else 
        //         {
        //             outFile << " no path !" << std::endl;
        //             new_random_objective();
        //         }

        //     }
        // }

        //only explore nodes when the next objective is not connected
        // if its not connected explore, find k closest, try the furthest non colliding

        // features.current_node = features.objective_nodes.back();
        // outFile << "number of nodes to explore " << features.nodes << std::endl;
        // outFile << "reached objective " << features.current_node << std::endl;
        // features.objective_nodes.pop_back();

        // outFile << "Exploring the map" << std::endl;
        // int k=8;
        // std::vector<int> indices(features.nodes);
        // std::vector<float> distances(features.nodes);

        // float cx = features.player1->x;
        // float cy = features.player1->y;

        // std::iota(indices.begin(), indices.end(), 0);
        // std:            [&distances](float i1, float i2) { return distances[i1] < distances[i2]; });

        // Return the closest k nodes
        // std::vector<int> min_nodes(indices.begin(), indices.begin() + k);

        // draw rays
        // std::vector<int> no_collide_neighbors;
        
        // for (int i : min_nodes)
        // {
        //     vec node = features.node_positions[i];
        //     traceresult_s tr;
        //     hook_util.resolver->TraceLine(features.node_positions[features.current_node], node, features.player1->base_address, true, &tr);


        //     if no collision add it to the graph
        //     outFile << i << " collided : " << tr.collided << " explored : " << features.connected_nodes[i] << std::endl;
        //     if (!tr.collided)
        //     {
        //         check if its already in the graph
        //         outFile << " " << features.connected_nodes[i] << std::endl;
        //         if (i < features.nodes)
        //         {
        //             outFile << "adding " << i << " to no collide neighbors" << std::endl;
        //             no_collide_neighbors.push_back(i);

        //         } else {
        //             outFile << "invalid node " <<std::endl;
        //         }
        //     }
        // }

        // set a objective to the furthest non colliding k-node
        // stacks in objectives to next tick we see this node in the graph and move to it
        // initiate exploration graph
        // if (no_collide_neighbors.size() > 0)
        // {
        //     outFile << "selecting a visible neighbor" << std::endl;
        //     std::random_device rd;
        //     std::mt19937 gen(rd());
        //     std::uniform_int_distribution<> dist(0, no_collide_neighbors.size()-1);
        //     int rndindx = dist(gen);
        //     if (rndindx >= 0 && rndindx < no_collide_neighbors.size())
        //     {
        //         outFile << no_collide_neighbors.size() << " " << rndindx << std::endl;
        //         if (features.connected_nodes[rndindx] == 0)
        //         {
        //         features.objective_nodes.push_back(rndindx);
        //         features.node_adjacency_mat[rndindx][features.current_node] = 1;
        //         features.node_adjacency_mat[features.current_node][rndindx] = 1;
        //         features.connected_nodes[rndindx] = 1;
        //     }
        //    }
        // } else 
        // {
        //     outFile << " no path !" << std::endl;
        // }

        // int connected = 0;
        // for(int c : features.connected_nodes)
        // {
        //     if (c > 0)
        //     {
        //         connected ++;
        //     }
        // }
        // int graph_explored = (float)(connected)/(float)(features.connected_nodes.size());
        // if (features.objective_nodes.empty())
        // {
        //     outFile << "no objective !! or fully mapped path";
        // }
        // for (int o : features.objective_nodes)
        // {
        //     vec pos = features.node_positions[o];
        //     outFile << o << " x: " << pos.x << " y: " << pos.y << std::endl;
        // }
        // outFile << "graph explored " << connected <<" / " << features.connected_nodes.size() << " : %" << graph_explored << std::endl;

    // }   

//     traceresult_s tr;

//     hook_util.resolver->Target_Ray_Trace(features.node_positions[features.objective_nodes.back()],&tr);
//     outFile << features.node_positions[features.objective_nodes.back()].x << " " << features.node_positions[features.objective_nodes.back()].y << " " << features.objective_nodes.back() << std::endl;
//     if (!tr.collided)
//     {
//         vec player_pos = {features.player1->x, features.player1->y, features.player1->z};
//         vec angular_displacement = GetRayAngle(player_pos, features.node_positions[features.objective_nodes.back()]);
//         angular_displacement.x += 180;
//         if (angular_displacement.x + 360) {
//             angular_displacement.x -= 360;
//         }
//         features.player1->set_yaw_pitch(angular_displacement.x, angular_displacement.y);

//         hook_util.interface->Keyboard_Event(sdl_keys.SDLK_w, hook_util.interface->sdl_util.SDL_KEYDOWN,1);
//     } else {
//         hook_util.interface->Keyboard_Event(sdl_keys.SDLK_w, hook_util.interface->sdl_util.SDL_KEYUP,0);
//     }


//     //outFile << "position : " << features.player1->x << " " << features.player1->y << std::endl;
//   transform(features.node_positions.begin(),
//                         features.node_positions.end(),
//                         distances.begin(), [cx,cy](vec v) { return std::sqrt(pow(v.x-cx,2)+pow(v.y-cy,2));});

//         // Sort the indices based on the corresponding values in vec
//         std::partial_sort(indices.begin(), indices.begin() + k, indices.end(),
//               outFile.close();
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
