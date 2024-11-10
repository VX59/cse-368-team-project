#include <cmath>
#include "../Feature_Resolver.h"

void WorldToScreen(float x, float y, float z, float *MVP, int screenWidth, int screenHeight, float *screenX, float *screenY);
float RadiansToDegrees(float rads);
float GetVectorDistance(vec v1, vec v2);
vec GetRayAngle(vec from, vec to);
