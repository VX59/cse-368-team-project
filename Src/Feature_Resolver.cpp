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
void Entity_Tracker::Resolve_Dynamic_Entities()
{
    this->features->player1->resolve_attributes();
    for (dynamic_ent *e : this->features->dynamic_entities)
    {
        e->resolve_attributes();
        
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