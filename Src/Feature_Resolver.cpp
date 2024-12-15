#include "Feature_Resolver.h"
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <random>
#include <iostream>

// resolves the player entity list
void Entity_Tracker::Update_Player_Entities()
{
    this->features->player1->resolve_attributes();
    for (dynamic_ent *e : this->features->dynamic_entities)
    {
        e->resolve_attributes();

        vec o;
        o.x = e->position.x;
        o.y = e->position.y;
        o.z = e->position.z;
        this->Target_Ray_Trace(o,e->trace);
    }
}

// calculate tangent and normal collision vectors relative to the camera direction
// up and down vectors are absolute becuase relative vectors become unstable at pitch = -.1->.1 radians

void Entity_Tracker::TNB_Ray_Trace()
{
     // trace rays from the players head .. this is supposed to be for ac bots
    this->features->player1->resolve_attributes();

    vec from = this->features->player1->position;
    from.z += 5.5;

    vec to;
    double yaw = (this->features->player1->yaw-90) * (M_PI/180.f);
    double pitch = (this->features->player1->pitch) * (M_PI/180.f);
    float epsilon = 0.15;
    // calculate 100 cube ray
    float limit = 1000.f;

    if (M_PI_2 - fabs(pitch) <= epsilon)
    {
        to.x = from.x;
        to.y = from.y;
        to.z = from.z + sin(pitch)*limit;        
    } else 
    {
        to.x = from.x + (cos(yaw) * cos(pitch)) * limit;
        to.y = from.y + (sin(yaw) * cos(pitch)) * limit;
        to.z = from.z + sin(pitch) * limit;
    }

    traceresult_s *tr = &(this->features->rays[0]);
    
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);
    // right
    to.x = from.x + (cos(yaw+M_PI_2)) * limit;
    to.y = from.y + (sin(yaw+M_PI_2)) * limit;
    to.z = from.z;
    tr = &(this->features->rays[1]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);

    // back
    if (M_PI_2 - fabs(pitch) <= epsilon)
    {
        to.x = from.x;
        to.y = from.y;
        to.z = from.z - sin(pitch)*limit;        
    } else 
    {
        to.x = from.x - (cos(yaw) * cos(pitch)) * limit;
        to.y = from.y - (sin(yaw) * cos(pitch)) * limit;
        to.z = from.z - sin(pitch) * limit;
    }
    tr = &(this->features->rays[2]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);

    // left
    to.x = from.x - (cos(yaw+M_PI_2)) * limit;
    to.y = from.y - (sin(yaw+M_PI_2)) * limit;
    to.z = from.z;
    tr = &(this->features->rays[3]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);
}

void Entity_Tracker::Target_Ray_Trace(vec target, traceresult_s *tr)
{
    vec from = this->features->player1->position;
    from.z += 5;
    
    this->TraceLine(from, target, this->features->player1->base_address, true, tr);
}

void Entity_Tracker::Add_Node(vec position, int idx)
{
    features->node_positions[idx] = position;
    features->connected_pool[idx] = 1;
    features->free_pool[idx] = 0;
    features->nodes ++;
    features->free_nodes --;
}

void Entity_Tracker::Remove_Node(int idx)
{
    vec default_pos = {1e7,1e7,1e7};

    // unadd the node
    for (int i = 0; i < features->node_adjacency_mat.size(); i++)
    {
        features->node_adjacency_mat[idx][i] = 0;
        features->node_adjacency_mat[i][idx] = 0;
    }

    features->node_positions[idx] = default_pos;
    features->connected_pool[idx] = 0;
    features->free_pool[idx] = 1;
    features->nodes --;
    features->free_nodes ++;   
}

int Entity_Tracker::Path_Find(int S, int T)
{
    std::vector<std::vector<int>> Graph = features->node_adjacency_mat;
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

        for (long unsigned int i = 0; i < features->node_adjacency_mat[current].size(); i++)
        {            
            int n = features->node_adjacency_mat[current][i] | features->node_adjacency_mat[i][current];            
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
        if (node < features->nodes && node != -1)
        {
            features->objective_nodes.push_back(node);
            i++;
        }
    }
    features->objective_nodes.pop_back();
    features->objective_is_path = true;
    return i;
}
