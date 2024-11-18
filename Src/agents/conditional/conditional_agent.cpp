#include "conditional_agent.h"
#include "../math_helpers.h"
#include <iostream>
#include <cmath>

void ConditionalAgent::run() {
    vec player1Position = {features->player1->x, features->player1->y, features->player1->z};

    // This will be cleaned up, we just aim at the closest entity if they are
    // on-screen (to-do check traceline if there is something in-between)
    for (dynamic_ent *ent: *(features->dynamic_entities)) {
        // if player is on our team or dead don't aim at them
        if (ent->team == features->player1->team || ent->health <= 0) {
            continue;
        }

        // seeing if enemy is on our screen
        float screenX, screenY = 0;
        WorldToScreen(ent->x, ent->y, ent->z, features->mvpmatrix, features->screenw, features->screenh, &screenX, &screenY);
        bool onScreen = (0 < screenX && screenX < features->screenw) && (0 < screenY && screenY < features->screenh);

        // seeing if they are actually visible
        vec enemyPosition = {ent->x, ent->y, ent->z};
        float enemyDistance = GetVectorDistance(player1Position, enemyPosition);
        float traceDistance = GetVectorDistance(player1Position, ent->trace->end);
        bool isVisible = traceDistance - 2.0f < enemyDistance && enemyDistance < traceDistance + 2.0f;

        if (onScreen && isVisible) {
            vec player1Position = {features->player1->x, features->player1->y, features->player1->z};
            vec otherPlayerPosition = {ent->x, ent->y, ent->z};
            vec angle = GetRayAngle(player1Position, otherPlayerPosition);
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