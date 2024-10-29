#include "Feature_Resolver.h"
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <random>

// resolves the player entity list
void Feature_Resolver::Resolve_Dynamic_Entities()
{
    this->features->player1->resolve_attributes();

    for (dynamic_ent *e : this->features->dynamic_entities)
    {
        e->resolve_attributes();
    }
}

void Feature_Resolver::Resolve_Static_Entities()
{
    for (static_ent *e : this->features->static_entities)
    {
        e->resolve_attributes();
    }
}

// calculate tangent and normal collision vectors relative to the camera direction
// there are technically 3 but we will make 5
void Feature_Resolver::TNB_Ray_Trace()
{
     // trace rays from the players head .. this is supposed to be for ac bots
    vec from;
    this->features->player1->resolve_attributes();

    from.x = this->features->player1->x;
    from.y = this->features->player1->y;
    from.z = this->features->player1->z+5.5; // player height ,, we can reverse camera1 but this is the same for now


    vec to;
    double yaw = (this->features->player1->yaw-90) * (M_PI/180.f);
    double pitch = (this->features->player1->pitch) * (M_PI/180.f);

    // calculate 100 cube ray
    float limit = 100.f;

    // forwards
    to.x = from.x + (cos(yaw) * cos(pitch)) * limit;
    to.y = from.y + (sin(yaw) * cos(pitch)) * limit;
    to.z = from.z + sin(pitch) * limit;
    traceresult_s *tr = &(this->features->rays[0]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);
    // right
    to.x = from.x + (cos(yaw+M_PI_2)) * limit;
    to.y = from.y + (sin(yaw+M_PI_2)) * limit;
    to.z = from.z;
    tr = &(this->features->rays[1]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);

    // back
    to.x = from.x -(cos(yaw) * cos(pitch)) * limit;
    to.y = from.y -(sin(yaw) * cos(pitch)) * limit;
    to.z = from.z -sin(pitch) * limit;
    tr = &(this->features->rays[2]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);

    // left
    to.x = from.x - (cos(yaw+M_PI_2)) * limit;
    to.y = from.y - (sin(yaw+M_PI_2)) * limit;
    to.z = from.z;
    tr = &(this->features->rays[3]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);

    // up
    to.x = from.x + (cos(pitch+M_PI_2)) * limit;
    to.y = from.y + (cos(pitch+M_PI_2)) * limit;
    to.z = from.z + sin(pitch+M_PI_2) * limit;
    tr = &(this->features->rays[4]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);

    // down
    to.x = from.x - (cos(pitch+M_PI_2)) * limit;
    to.y = from.y - (cos(pitch+M_PI_2)) * limit;
    to.z = from.z - sin(pitch+M_PI_2) * limit;
    tr = &(this->features->rays[5]);
    this->TraceLine(from, to, this->features->player1->base_address, true, tr);
}