#pragma once
#include <unistd.h>
#include <vector>
#include <random>
#include "sdl_lib.h"

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
    }rel_s_offsets;
    

    __uint64_t base_address;
    traceresult_s *trace;
    entity(__uint64_t base)
    {
        base_address = base;
    }

};

struct static_ent : entity
{
    __uint16_t x, y, z;
    __uint8_t type;

    static_ent(__uint64_t base) : entity(base) { };

    void resolve_attributes()
    {
        x = *(__uint16_t*)(base_address + rel_s_offsets.x);
        y = *(__uint16_t*)(base_address + rel_s_offsets.y);
        z = *(__uint16_t*)(base_address + rel_s_offsets.z);
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
    float x;
    float y;
    float z;
    float yaw;
    float pitch;

    dynamic_ent(__uint64_t base) : entity(base) { };

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
        x = *(float*)(base_address + rel_d_offsets.x);
        y = *(float*)(base_address + rel_d_offsets.y);
        z = *(float*)(base_address + rel_d_offsets.z);
        yaw = *(float*)(base_address + rel_d_offsets.yaw);
        pitch = *(float*)(base_address + rel_d_offsets.pitch);
    }
};

struct Features
{
    traceresult_s rays[4];
    dynamic_ent *player1;
    std::vector<dynamic_ent*> *dynamic_entities;
    std::vector<static_ent*> *static_entities;

    // these fields allow us to turn 3D XYZ coordinates into on-screen 2D
    // coordinates (ESP/aimbot)
    int screenw;
    int screenh;
    float *mvpmatrix;

    // for exploration
    int random_nodes = 2048;
    int nodes = random_nodes;
    std::vector<vec> node_positions;
    std::vector<int> connected_nodes;
    std::vector<std::vector<int>> node_adjacency_mat;
    std::vector<int> objective_nodes;
    int current_node;
    bool objective_is_path = false;
};

class Feature_Resolver
{
public:
    Features *features;

    void (*TraceLine)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr);
    void (*patricle_trail)(int type, int fade, vec s, vec e);

    // resolve player entity features including player1
    void Resolve_Dynamic_Entities();
    // traces rays from player1
    void TNB_Ray_Trace();
    void Target_Ray_Trace(vec target, traceresult_s *tr);

    Feature_Resolver(__uint64_t p1, __uint64_t players, __uint64_t ents, Features *f)
    {
        dynamic_ent *player1 = new dynamic_ent((__uint64_t)(*(__uint64_t **)p1));
        features = f;
        features->player1 = player1;
        features->player1->resolve_attributes();

        __uint64_t players_data_address = *(__uint64_t*)players;
        __uint32_t players_size = *(__uint32_t*)(players+0xc);

        std::vector<dynamic_ent*> *dynamic_ents = new std::vector<dynamic_ent*>;

        for (__uint32_t i = 0; i < players_size; i++)
        {
            __uint64_t player_address = *(__uint64_t*)(players_data_address+0x8*i);
            if (player_address != 0x0)
            {                
                dynamic_ent *player = new dynamic_ent(player_address);
                traceresult_s *tr = new traceresult_s;
                player->trace = tr;
                dynamic_ents->push_back(player);
            }

        }
        features->dynamic_entities = dynamic_ents;

        __uint64_t ents_data_address = *(__uint64_t*)ents;
        __uint32_t ents_size = *(__uint32_t*)(ents+0xc);

        std::vector<static_ent*> *static_ents = new std::vector<static_ent*>;

        for (__uint32_t i = 0; i < ents_size; i++)
        {
            __uint64_t ent_address = ents_data_address+28*i;
            if (ent_address != 0x0)
            {                
                static_ent *ent = new static_ent(ent_address);
                ent->resolve_attributes();
                vec o;
                o.x = ent->x;
                o.y = ent->y;
                o.z = ent->z; 
                this->features->node_positions.push_back(o);
                static_ents->push_back(ent);
                this->features->connected_nodes.push_back(0);
            }

        }
        features->static_entities = static_ents;

        vec start;
        start.x=features->player1->x;
        start.y=features->player1->y;
        start.z=features->player1->z;

        features->node_positions.push_back(start);
        features->current_node = features->node_positions.size()-1;
        features->connected_nodes[features->current_node] = 1;
        features->objective_nodes.push_back(features->current_node);

        // initiate exploration graph
        std::random_device rd;
        std::mt19937 gen(rd());
        
        float bounds = 75;
        std::uniform_real_distribution<float> xdist(features->player1->x-bounds,features->player1->x+bounds);
        std::uniform_real_distribution<float> ydist(features->player1->y-bounds,features->player1->y+bounds);        
        std::uniform_real_distribution<float> zdist(0,9);
        int nodes = features->random_nodes;
        // n random points on the map
        for(int i = 0; i < nodes; i++)
        {
            vec rndv = {xdist(gen), ydist(gen), zdist(gen)};
            while (rndv.x > 200 || rndv.y > 200 || rndv.x == 0 || rndv.y == 0)
            {
                rndv = {xdist(gen), ydist(gen), zdist(gen)};
            }
            features->node_positions.push_back(rndv);
            features->connected_nodes.push_back(0);
        }

        features->nodes = features->node_positions.size();
        std::vector<std::vector<int>> mat(features->node_positions.size(), std::vector<int>(features->node_positions.size(),0));
        features->node_adjacency_mat = mat;
    };
};