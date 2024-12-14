#include "math_helpers.h"

void WorldToScreen(vec p, float *MVP, int screenWidth, int screenHeight, float *screenX, float *screenY) {
    float mvpX = MVP[0] * p.x + MVP[4] * p.y + MVP[8] *  p.z + MVP[12];
    float mvpY = MVP[1] * p.x + MVP[5] * p.y + MVP[9] *  p.z + MVP[13];
    float mvpZ = MVP[2] * p.x + MVP[6] * p.y + MVP[10] * p.z + MVP[14];
    float mvpW = MVP[3] * p.x + MVP[7] * p.y + MVP[11] * p.z + MVP[15];

    float ndcX = mvpX / mvpW;
    float ndcY = mvpY / mvpW;
    float ndcZ = mvpZ / mvpW;

    if (ndcZ < 0.0f || ndcZ > 1.0f) {
        *screenX = -1;
        *screenY = -1;
    }
    else {
        *screenX = (ndcX + 1) * 0.5f * screenWidth;
        *screenY = (1 - ndcY) * 0.5f * screenHeight;
    }
}

float RadiansToDegrees(float rads) {
    return rads *(180.0f / M_PI);
}

float GetVectorDistance(vec v1, vec v2) {
    return sqrt(pow(v2.x - v1.x, 2) + pow(v2.y - v1.y, 2) + pow(v2.z - v1.z, 2));
}

// source: Mr Ripperoni
vec GetRayAngle(vec from, vec to) {
    vec results = {0.0f, 0.0f, 0.0f};
    results.x = RadiansToDegrees(-atan2f(to.x - from.x, to.y - from.y));
    if (results.x <= 0.0f) {
        results.x += 360.0f;
    }

    results.y = RadiansToDegrees(asinf((to.z - from.z) / GetVectorDistance(from, to)));
    return results;
}