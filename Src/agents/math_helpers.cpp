#include "math_helpers.h"

void WorldToScreen(float x, float y, float z, float *MVP, int screenWidth, int screenHeight, float *screenX, float *screenY) {
    float mvpX = MVP[0] * x + MVP[4] * y + MVP[8] * z + MVP[12];
    float mvpY = MVP[1] * x + MVP[5] * y + MVP[9] * z + MVP[13];
    float mvpZ = MVP[2] * x + MVP[6] * y + MVP[10] * z + MVP[14];
    float mvpW = MVP[3] * x + MVP[7] * y + MVP[11] * z + MVP[15];

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