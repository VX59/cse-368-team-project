#include "hunter_agent.h"
#include "agents/math_helpers.h"
float Hunter_Agent::flat_distance(vec v1, vec v2)
{
    return sqrt(pow(v2.x - v1.x, 2) + pow(v2.y - v1.y, 2));
}

std::vector<int> Hunter_Agent::sort_nodes(vec from)
{  
    int pool_size = tracker->features->connected_pool.size();
    int curr_node = tracker->features->current_node;

    std::vector<float> distances;
    for (int i = 0; i < pool_size; i++)
    {
        if (i == curr_node)
        {
            distances.push_back(1e7);
            continue;
        }
        distances.push_back(flat_distance(from, tracker->features->node_positions[i]));
    }
    
    std::vector<int> indices(pool_size);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&distances](float i1, float i2) {
        return distances[i1] < distances[i2];
    });

    return indices;
}


void Hunter_Agent::Update_Objective_Velocity()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);

    if (tracker->features->objective_nodes.empty())
    {
        outFile << old_objective_dist << "objectives is empty" << std::endl;
        old_objective_dist = 0;
        objective_dist = 0;
        tracker->Update_Player_Entities();

    } else {
        int obj_node = tracker->features->objective_nodes.back();
        vec obj_node_pos = tracker->features->node_positions[obj_node];
        old_objective_dist = flat_distance(obj_node_pos,tracker->features->player1->position);
        tracker->Update_Player_Entities();
        objective_dist = flat_distance(obj_node_pos,tracker->features->player1->position);
    }

    objective_vel = old_objective_dist - objective_dist;
    outFile << "objective velocity " << fabs(objective_vel) << " objective dist " << objective_dist << std::endl;
    outFile.close();
}

void Hunter_Agent::Update_Target_Position()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);
    outFile << "evaluating enemy positions" << std::endl;

    // picking the closest target
    std::vector<float>distances;

    for (dynamic_ent *player_ent : tracker->features->dynamic_entities)
    {
        distances.push_back(flat_distance(tracker->features->player1->position, player_ent->position));
    }

    if (!distances.empty())
    {
        std::vector<int> indices(distances.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&distances](float i1, float i2) {
            return distances[i1] < distances[i2];
        });
        int min_player = indices[0];
        tracker->features->target = tracker->features->dynamic_entities[min_player]->position;

        outFile << "targeting enemy #" << min_player << std::endl;
    } else
    {
        outFile << "noone to target" << std::endl;
    }

    outFile.close();
}

void Hunter_Agent::Discover_Routes(std::vector<int> min_nodes)
{
    int curr_node = tracker->features->current_node;
    int pool_size = tracker->features->connected_pool.size();
    for (int i = 0; i < min_nodes.size(); i++)
    {
        int prospect = min_nodes[i];
        if (prospect < pool_size && prospect != curr_node)
        {
            vec node = tracker->features->node_positions[prospect];
            if (node.x == -1) continue;
            traceresult_s tr;
            tracker->Target_Ray_Trace(node, &tr);

            if (!tr.collided)
            {
                if (tracker->features->connected_pool[prospect] == 0)
                {
                    tracker->features->node_adjacency_mat[prospect][curr_node] = 1;
                    tracker->features->node_adjacency_mat[curr_node][prospect] = 1;
                }
            }
        }
    }
}

void Hunter_Agent::Prune_Graph()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);
   // find k nodes with highest density;
    int k = sqrtf(tracker->features->connected_pool.size());
    std::vector<float> densities; 
    vec default_pos = {1e7,1e7,1e7};

    int curr_node = tracker->features->current_node;
    int pool_size = tracker->features->connected_pool.size();
    vec curr_pos = tracker->features->player1->position;

    outFile << "locating densest nodes and removing them" << std::endl;
    for (int i = 0; i < pool_size; i++)
    {
        if (i != curr_node)
        {
            int card = 0;
            float dist = 0.0f;
            for(int j = 0; j < pool_size; j++)
            {
                if (tracker->features->connected_pool[j])
                {
                    if (tracker->features->node_adjacency_mat[i][j] != 0) card += 1;
                    dist += pow(flat_distance(tracker->features->node_positions[i], tracker->features->node_positions[j]),2);
                }
            }
            densities.push_back(dist*card);
        }
    }
    std::vector<int> indices(densities.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&densities](float i1, float i2) {
        return densities[i1] < densities[i2];
    });
    
    std::vector<int> min_nodes(indices.begin(), indices.begin() + k);
    outFile << "removoing " << k << " nodes" << std::endl;
    for (int i = 0; i < k; i++)
    {
        int suspect = min_nodes[i];
        tracker->Remove_Node(suspect);
    }
}

void Hunter_Agent::Scan_Environment(int k, bool add_obj)
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);

    outFile << "scanning environment " << std::endl;

    vec from = tracker->features->player1->position;
    from.z += 5.5;
    
    int curr_node = tracker->features->current_node;
    int pool_size = tracker->features->connected_pool.size();
    vec curr_pos = tracker->features->player1->position;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> ryaw(0,360);

    vec to;
    vec default_pos = {1e7,1e7,1e7};

    for (int i = 0; i < k; i++)
    {
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
            
            tracker->Target_Ray_Trace(to, &tr);

            vec delta = {(tr.end.x-from.x)/2, (tr.end.y-from.y)/2, from.z};
            scan_distance = sqrtf(pow(delta.x,2) + pow(delta.y,2));

            to = {from.x + delta.x, from.y + delta.y, from.z};
        }

        auto it = std::find(tracker->features->free_pool.begin(), tracker->features->free_pool.end(), 1);
        int idx = std::distance(tracker->features->free_pool.begin(), it);

        if (it != tracker->features->free_pool.end())
        {
            tracker->Add_Node(to, idx, 1);
            tracker->features->node_adjacency_mat[idx][curr_node] = 1;
            tracker->features->node_adjacency_mat[curr_node][idx] = 1;
            if (add_obj)
            {
                outFile << "setting objective to " << idx << std::endl;
                tracker->features->objective_nodes.push_back(idx);
            }
            outFile << "allocated node " << idx << std::endl;
        } else
        {
            outFile << "unable to allocate the node not spots left" << std::endl;
            break;
        }
    }
    outFile.close();
}

void Hunter_Agent::Follow_Path()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);

    if (jump_status)
    {
        interface->Keyboard_Event(interface->sdl_keys.SDLK_SPACE, interface->sdl_util.SDL_KEYDOWN, interface->sdl_util.SDL_PRESSED);
    } else
    {
        interface->Keyboard_Event(interface->sdl_keys.SDLK_SPACE, interface->sdl_util.SDL_KEYUP, interface->sdl_util.SDL_RELEASED);
    }

    int curr_node = tracker->features->current_node;
    int pool_size = tracker->features->connected_pool.size();
    vec curr_pos = tracker->features->player1->position;

    vec player_pos = {curr_pos.x, curr_pos.y, curr_pos.z};

    int target_idx = tracker->features->objective_nodes.back();
    
    vec target = tracker->features->node_positions[target_idx];
    target.z =  curr_pos.z;
    vec angular_displacement = GetRayAngle(player_pos,target);
    angular_displacement.x += 180;
    if (angular_displacement.x > 360) {
        angular_displacement.x -= 360;
    }

    tracker->features->player1->set_yaw_pitch(angular_displacement.x, angular_displacement.y);

    interface->Keyboard_Event(interface->sdl_keys.SDLK_w, interface->sdl_util.SDL_KEYDOWN,1);

    outFile << "P1 pos x: "<< curr_node << " " << curr_pos.x << " y: " << curr_pos.y << " z: " << curr_pos.z << std::endl;   
    outFile << "Enemy pos: " << tracker->features->target.x << " " << tracker->features->target.y << std::endl; 
    outFile.close();   
}

void Hunter_Agent::Navigate()
{    
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
   
    Update_Objective_Velocity();
    Update_Target_Position();

    int curr_node = tracker->features->current_node;
    int pool_size = tracker->features->connected_pool.size();
    vec curr_pos = tracker->features->player1->position;
    vec default_pos = {1e7,1e7,1e7};

    jump_status = false;
    outFile << "checking if reached objective" << std::endl;

    if (objective_dist < prox)
    {
        if (!tracker->features->objective_nodes.empty())
        {
            tracker->features->current_node = tracker->features->objective_nodes.back();
            tracker->features->objective_nodes.pop_back();      
        }
        
        curr_node = tracker->features->current_node;
        outFile << "reached an objective , current node " << curr_node << std::endl;
        if (curr_node < 0)
        {
            outFile << "invalid current node" << std::endl;
            return;
        }

        outFile << "probing the graph" << std::endl;
        int k = 4;
        
        std::vector<int> indices = sort_nodes(tracker->features->node_positions[curr_node]);
        std::vector<int> min_nodes(indices.begin(), indices.begin() + k);
        
        Discover_Routes(min_nodes);

        if (tracker->features->objective_nodes.empty())
        {

            if (tracker->features->free_nodes < k)
            {   
                outFile << "pruning graph" << std::endl;
                Prune_Graph();
            }         
            Scan_Environment(k, false);
        }        
    }

    if (objective_dist < pow(prox,2))
    {
        if (tracker->features->connected_pool[curr_node] == -1)
        {
            outFile << "Reached a jump node" << std::endl;
            
            jump_status = true;
        }
    }
    
    outFile << "checking if stuck on a path" << std::endl;    
    if (objective_vel >= 0 && objective_vel < 0.0001 && objective_dist >= prox && !tracker->features->objective_nodes.empty())
    {
        // first place a jump node request and jump

        if(!jump_node_request)
        {
            outFile << "Sending a Jump Node request " << std::endl;
            jump_node_request = true;
            jump_status = true;
            jump_delta = objective_vel;
            if (tracker->features->free_nodes == 0)
            {   
                Prune_Graph();
            }      
            auto it = std::find(tracker->features->free_pool.begin(), tracker->features->free_pool.end(), 1);
            jump_node_idx = std::distance(tracker->features->free_pool.begin(), it)-1;

            tracker->Add_Node(tracker->features->player1->position, jump_node_idx, -1); // type 7 for jump node
        } else
        {
            outFile << "Processing a Jump Node request " << std::endl;
        }
    }

    outFile << "checking jump node request" << std::endl;

    if (jump_node_request)
    {
        jump_delta += objective_vel;
        if (jump_delta > 0.5) jump_node_status = true;

        if (jump_tick_counter == 64)
        {
            if (jump_node_status)
            {
                outFile << "Jump Node Request Accepted .. Recongiguring Graph" << std::endl;
                // in the adjacency matrix connect the current node to the jump node and the objective to the jump node
                // also remove the connection between the current and objective nodes
                outFile << "1st" << std::endl;
                if (jump_node_idx > -1 && curr_node > -1)
                {
                    tracker->features->node_adjacency_mat[jump_node_idx][curr_node] = -1;
                    tracker->features->node_adjacency_mat[curr_node][jump_node_idx] = -1;
                }
                outFile << "2nd" << std::endl;

                if (jump_node_idx > -1 && !tracker->features->objective_nodes.empty())
                {
                    tracker->features->node_adjacency_mat[jump_node_idx][tracker->features->objective_nodes.back()] = -1;
                    tracker->features->node_adjacency_mat[tracker->features->objective_nodes.back()][jump_node_idx] = -1;
                    tracker->features->objective_nodes.push_back(jump_node_idx);
                }
                outFile << "3rd" << std::endl;

                if (curr_node > -1 && !tracker->features->objective_nodes.empty())
                {
                    tracker->features->node_adjacency_mat[tracker->features->objective_nodes.back()][curr_node] = 0;
                    tracker->features->node_adjacency_mat[curr_node][tracker->features->objective_nodes.back()] = 0;
                }

            } else
            {
                outFile << "evaluating objective velocity " << std::endl;

                outFile << "jump_node_idx " << jump_node_idx << std::endl;
                if (jump_node_idx >= 0)
                {
                    tracker->Remove_Node(jump_node_idx);
                }
                jump_node_idx = -1;
                jump_delta = 0;

                if (!tracker->features->objective_nodes.empty())
                {
                    int node = tracker->features->objective_nodes.back();
                    tracker->features->objective_nodes.clear();
                    // otherwise delete the jump node and proceed normally

                    outFile << "cleared objectives .. proceed to find a new path" << std::endl;            
                    
                    outFile << "PRUNING NODE " << node << " current node " << tracker->features->current_node << std::endl;
                    tracker->Remove_Node(node);
                    
                    Scan_Environment(1, true);
                }
            }

            jump_status = false;
            jump_node_request = false;
            jump_node_status = false;
            jump_delta = 0;
            jump_node_idx = -1;
            jump_tick_counter = 0;

        } else{
            jump_tick_counter ++;
        }
    }

    outFile << "checking if needs a new path" << std::endl;

    if (!tracker->features->objective_nodes.empty())
    {
        outFile << tracker->features->objective_nodes.size() << " OBJECTIVES" << std::endl;
        if (tracker->features->objective_nodes.size() > pool_size)
        {
            outFile << "invalid memory access" << std::endl;
            return;
        } else
        {
            for (int o : tracker->features->objective_nodes)
            {
                vec pos = tracker->features->node_positions[o];
                outFile << o << " x: " << pos.x << " y: " << pos.y << "z: " << pos.z << std::endl;
            }
        }

    } else
    {
        interface->Keyboard_Event(interface->sdl_keys.SDLK_w, interface->sdl_util.SDL_KEYUP, interface->sdl_util.SDL_RELEASED);
        outFile << "selecting a path" << std::endl;
        
        std::vector<int> connected_nodes;
        for (int i = 0; i < tracker->features->nodes; i++)
        {
            if (tracker->features->connected_pool[i] > 0 && i != curr_node)
            {
                connected_nodes.push_back(i);
            }
        }
        if (connected_nodes.empty())
        {
            outFile << "no connected nodes" << std::endl;
            return;
        }

        outFile << "current node " << curr_node << std::endl;

        vec cur_pos, nex_pos;
        int best_pick;

        outFile << "target position " << tracker->features->target.x << " " << tracker->features->target.y << std::endl;
        if(tracker->features->target.x != default_pos.x)
        {
            outFile << "estimating path to target" << std::endl;

            std::vector<int> min_nodes = sort_nodes(tracker->features->target);

            best_pick = *min_nodes.begin();
            outFile << "next node is " << best_pick << std::endl;
        } else
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, connected_nodes.size()-1);
            best_pick = dist(gen);
        }
        
        if (best_pick != curr_node)
        {
            nex_pos = tracker->features->node_positions[best_pick];

            outFile << "BFSing from " << curr_node << " to " << best_pick << std::endl;
            int i = tracker->Path_Find(curr_node, best_pick);   
            if (i == -1)
            {

                std::vector<int> min_nodes = sort_nodes(tracker->features->target);

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(0, connected_nodes.size()-1);
                
                int j = 1;
                while(i == -1)
                {
                    if (j = min_nodes.size())
                    {
                        break;
                    }

                    best_pick = min_nodes[j];  
                    int i = tracker->Path_Find(curr_node, best_pick);
                    j++;
                }
                outFile << "path length " << i << std::endl;
            }
        }  
    }

    outFile << "checking if path is set to follow" << std::endl;

    if (!tracker->features->objective_nodes.empty())
    {
        Follow_Path();
    } else
    {
        outFile << "Divergent Path : Cant Find Target" << std::endl;
        // scan
        if (tracker->features->free_nodes == 0)
        {   
            Prune_Graph();
        }         
        Scan_Environment(1, true);
    }
    float enemy_dist = flat_distance(curr_pos, tracker->features->target);

    outFile << "Connected Nodes " << tracker->features->nodes << " Free Nodes " << tracker->features->free_nodes << std::endl;
    outFile << "Target Distance " << enemy_dist << std::endl;

    outFile.close();
}