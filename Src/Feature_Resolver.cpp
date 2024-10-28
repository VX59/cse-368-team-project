#include "Feature_Resolver.h"
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <random>

void Feature_Resolver::Ray_Trace(float yaw_offset, float pitch_offset)
{
     // trace rays from the players head .. this is supposed to be for ac bots
    vec from;
    this->player1->resolve_attributes();

    from.x = this->player1->x;
    from.y = this->player1->y;
    from.z = this->player1->z+5.5; // player height ,, we can reverse camera1 but this is the same for now
    
    vec to;
    float yaw = (this->player1->yaw-90+yaw_offset) * (M_PI/180.0f);
    float pitch = (this->player1->pitch+pitch_offset) * (M_PI/180.0f);

    // calculate 100 cube ray
    float limit = 100;
    to.x = from.x + (float)(cos(yaw)*cos(pitch))*limit;
    to.y = from.y + (float)(sin(yaw)*cos(pitch))*limit;
    to.z = from.z + (float)(sin(pitch))*limit;
    
    int ray_index = yaw_offset/90;
    traceresult_s *tr = &(this->features->rays[ray_index]);

    this->TraceLine(from, to, this->player1->base_addresss, true, tr);
}