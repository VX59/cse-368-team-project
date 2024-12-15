#pragma once
#include <unistd.h>
#include <vector>
#include <random>
#include "sdl_lib.h"
#include <bits/stdc++.h>

struct vec
{
    union
    {
        struct { float x, y, z; };
        float v[3];
        int i[3];                               
    };
};

struct traceresult_s
{
     vec end;
     bool collided;
};

struct entity
{
    struct
    {
        __uint32_t name = 0x219;
        __uint32_t team = 0x320;
        __uint32_t health = 0x100;
        __uint32_t armor =  0x104;
        __uint32_t rifle_ammo = 0x154;
        __uint32_t pistol_ammo = 0x140;
        __uint32_t grenades = 0x152;
        __uint32_t pistol_ammo_reserve = 0x11C;
        __uint32_t rifle_ammo_reserve =  0x130;
        __uint32_t x = 0x2c;
        __uint32_t y = 0x30;
        __uint32_t z = 0x34;
        __uint32_t yaw = 0x38;
        __uint32_t pitch = 0x3c;
        __uint32_t roll = 0x40;
    }rel_d_offsets;

    struct 
    {
        __uint16_t x = 0x0;
        __uint16_t y = 0x2;
        __uint16_t z = 0x4;
        __uint16_t attr1 = 0x6;
        __uint8_t type = 0x8;
    } rel_s_offsets;
    

    __uint64_t base_address;
    traceresult_s *trace;
    entity(__uint64_t base)
    {
        base_address = base;
    }

};

struct static_ent : entity
{
    vec position;
    __uint8_t type;

    static_ent(__uint64_t base) : entity(base) { };

    void resolve_attributes()
    {   
        __uint16_t x,y,z;
        x = *(__uint16_t*)(base_address + rel_s_offsets.x);
        y = *(__uint16_t*)(base_address + rel_s_offsets.y);
        z = *(__uint16_t*)(base_address + rel_s_offsets.z);
        position.x = static_cast<float>(x);
        position.y = static_cast<float>(y);
        position.z = static_cast<float>(z);

        type = *(__uint8_t*)(base_address + rel_s_offsets.type);
    }
};

struct dynamic_ent : entity
{
    char name[260];
    int team;
    int health;
    int armor;
    int rifle_ammo;
    int pistol_ammo;
    int pistol_ammo_reserve;
    int rifle_ammo_reserve;
    int grenades;
    vec position;
    float yaw;
    float pitch;

    dynamic_ent(__uint64_t base) : entity(base) { };

    void set_health(int v) {
        *(int*)(base_address+rel_d_offsets.health) = v;
    }

    void set_rifle_ammo(int v) {
        *(int*)(base_address+rel_d_offsets.rifle_ammo) = v;
    }

    void set_yaw_pitch(float y, float p) {
        *(float*)(base_address+rel_d_offsets.yaw) = y;
        *(float*)(base_address+rel_d_offsets.pitch) = p;
    }

    void resolve_attributes()
    {
        char *memName = (char *)(base_address+rel_d_offsets.name);
        for (int i = 0; i < 260; i++) {
            name[i] = memName[i];
        }

        team = *(__uint64_t*)(base_address+rel_d_offsets.team);
        health = *(__uint64_t*)(base_address+rel_d_offsets.health);
        armor = *(__uint64_t*)(base_address + rel_d_offsets.armor);
        rifle_ammo = *(__uint64_t*)(base_address + rel_d_offsets.rifle_ammo);
        pistol_ammo = *(__uint64_t*)(base_address + rel_d_offsets.pistol_ammo);
        grenades = *(__uint64_t*)(base_address + rel_d_offsets.grenades);
        pistol_ammo_reserve = *(__uint64_t*)(base_address + rel_d_offsets.pistol_ammo_reserve);
        rifle_ammo_reserve = *(__uint64_t*)(base_address + rel_d_offsets.rifle_ammo);
        position.x = *(float*)(base_address + rel_d_offsets.x);
        position.y = *(float*)(base_address + rel_d_offsets.y);
        position.z = *(float*)(base_address + rel_d_offsets.z);
        yaw = *(float*)(base_address + rel_d_offsets.yaw);
        pitch = *(float*)(base_address + rel_d_offsets.pitch);
    }
};

struct Features
{   
    traceresult_s rays[4];
    dynamic_ent *player1;
    std::vector<dynamic_ent*> dynamic_entities;
    std::vector<static_ent*> static_entities;

    // these fields allow us to turn 3D XYZ coordinates into on-screen 2D
    // coordinates (ESP/aimbot)
    int screenw;
    int screenh;
    float *mvpmatrix;

    // for exploration
    int free_nodes = 2048;
    int nodes = 0;
    std::vector<vec> node_positions;
    std::vector<int> free_pool;
    std::vector<int> connected_pool;
    std::vector<std::vector<int>> node_adjacency_mat;
    std::vector<int> objective_nodes;
    int current_node;
    bool objective_is_path = false;

    vec target;
    int target_ent_idx;    
};

class Entity_Tracker
{
public:
    Features *features;
    void (*TraceLine)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr);
    void (*patricle_trail)(int type, int fade, vec s, vec e);
    void (*drawradarent)(float x, float y, float yaw, int col, int row, float iconsize, bool pulse, const char *label,...);
    // resolve player entity features including player1
    void Update_Player_Entities();
    // traces rays from player1
    void TNB_Ray_Trace();
    void Target_Ray_Trace(vec target, traceresult_s *tr);

    void Add_Node(vec position, int idx, int type);
    void Remove_Node(int idx);
    int Path_Find(int S, int T);

    Entity_Tracker(__uint64_t p1, __uint64_t players, __uint64_t ents, Features *F)
    {
        features = F;

        dynamic_ent *player1 = new dynamic_ent((__uint64_t)(*(__uint64_t **)p1));
        features->player1 = player1;
        features->player1->resolve_attributes();

        __uint64_t players_data_address = *(__uint64_t*)players;
        __uint32_t players_size = *(__uint32_t*)(players+0xc);
        
        features->dynamic_entities = std::vector<dynamic_ent*>();

        for (__uint32_t i = 0; i < players_size; i++)
        {
            __uint64_t player_address = *(__uint64_t*)(players_data_address+0x8*i);
            if (player_address != 0x0)
            {                
                dynamic_ent *player = new dynamic_ent(player_address);
                player->trace = new traceresult_s;
                player->resolve_attributes();
                features->dynamic_entities.push_back(player);
            }
        }

        __uint64_t ents_data_address = *(__uint64_t*)ents;
        __uint32_t ents_size = *(__uint32_t*)(ents+0xc);

        features->static_entities = std::vector<static_ent*>();
        const int ent_size = 28;

        for (__uint32_t i = 0; i < ents_size; i++)
        {
            __uint64_t ent_address = ents_data_address+ent_size*i;
            if (ent_address != 0x0)
            {                
                static_ent *ent = new static_ent(ent_address);
                ent->resolve_attributes();
                
                if (ent->position.x < 250 && ent->position.x > 0 && ent->position.y < 250 && ent->position.y > 0)
                {
                    std::vector<int> ignored_entity_types = {0,1,10,11,14,15,16,17,18};
                    auto it = std::find(ignored_entity_types.begin(), ignored_entity_types.end(), ent->type);
                    if (it == ignored_entity_types.end())
                    {
                        features->static_entities.push_back(ent);
                        //Add_Node(ent->position);
                    }
                }
            }
        }

        // mark unallocated nodes
        vec default_pos = {1e7,1e7,1e7};

        for (int i = 0; i < features->free_nodes+features->nodes; i++)
        {
            features->node_positions.push_back(default_pos);
            features->connected_pool.push_back(0);
            features->free_pool.push_back(1);
        }
        std::vector<std::vector<int>> mat(features->free_nodes, std::vector<int>(features->free_nodes,0));
        features->node_adjacency_mat = mat;

        Add_Node(player1->position, 0, 1);
    
        features->objective_nodes.push_back(0);
        features->target_ent_idx = -1;
    };
};  