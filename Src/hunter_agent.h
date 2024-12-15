#pragma once
#include <unistd.h>
#include <vector>
#include <random>
#include "sdl_lib.h"
#include <bits/stdc++.h>
#include "Feature_Resolver.h"
#include "Environment_Interaction.h"

class Hunter_Agent
{
public:
    Hunter_Agent(Entity_Tracker *T, Environment_Interaction *I)
    {
        std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log",std::ios::app);

        tracker = T;
        interface = I;

        if(!tracker->features->dynamic_entities.empty())
        {
            outFile << "searching for a target" << std::endl;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0,tracker->features->dynamic_entities.size()-1);
            
            int rndx = dist(gen);
            dynamic_ent target_ent = *(tracker->features->dynamic_entities[rndx]);            
            tracker->features->target_ent_idx = rndx;
            tracker->features->target = target_ent.position;
        } else
        {
            outFile << "no entities found to target .. proceeding to explore" << std::endl;
            vec default_pos = {1e7,1e7,1e7};
            tracker->features->target = default_pos;
        }
        outFile.close();

        jump_node_request = false;
        jump_node_status = false;
        jump_delta = 0;
        jump_node_idx = -1;
        jump_tick_counter = 0;
        jump_status = false;

    };

    void Navigate();

private:
    Entity_Tracker *tracker;
    Environment_Interaction *interface;

    float old_objective_dist, objective_dist, objective_vel;
    float prox = 2;

    bool jump_node_request;
    bool jump_node_status;
    bool jump_status;
    float jump_delta;
    float jump_node_idx;
    int jump_tick_counter;


    static float flat_distance(vec v1, vec v2);
    std::vector<int> sort_nodes(vec from);

    void Update_Objective_Velocity();
    void Update_Target_Position();
    void Discover_Routes(std::vector<int> min_nodes);
    void Prune_Graph();
    void Scan_Environment(int k, bool add_obj);
    void Follow_Path();
};