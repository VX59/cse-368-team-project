#include "conditional_agent.h"
#include "../math_helpers.h"
#include <iostream>
#include <cmath>

void ConditionalAgent::run() {
    // This will be cleaned up, we just aim at the closest entity if they are
    // on-screen (to-do check traceline if there is something in-between)
    for (dynamic_ent *ent: features->dynamic_entities) {
        // if player is on our team or dead don't aim at them
        if (ent->team == features->player1->team || ent->health <= 0) {
            continue;
        }

        // seeing if enemy is on our screen
        float screenX, screenY = 0;
        WorldToScreen(ent->position, features->mvpmatrix, features->screenw, features->screenh, &screenX, &screenY);
        bool onScreen = (0 < screenX && screenX < features->screenw) && (0 < screenY && screenY < features->screenh);

        // seeing if they are actually visible
        float enemyDistance = GetVectorDistance(features->player1->position, ent->position);
        float traceDistance = GetVectorDistance(features->player1->position, ent->trace->end);
        bool isVisible = traceDistance - 2.0f < enemyDistance && enemyDistance < traceDistance + 2.0f;

        if (onScreen && isVisible) {
            vec angle = GetRayAngle(features->player1->position, ent->position);
            angle.x += 180;
            if (angle.x + 360) {
                angle.x -= 360;
            }
            features->player1->set_yaw_pitch(angle.x, angle.y);
            break;
        }
    }
}

ConditionalAgent::ConditionalAgent(Features *features, Environment_Interaction *interface) {
    this->features = features;
    this->interface = interface;
}