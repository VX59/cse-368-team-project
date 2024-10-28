#include "player_ent.h"
#include <unistd.h>
#include <vector>

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
    }rel_offsets;

    __uint64_t base_address;
    entity(__uint64_t base)
    {
        base_address = base;
    }

};

struct static_ent : entity
{
    float x, y, z;
    int type;

    static_ent(__uint64_t base) : entity(base) { };

    void resolve_attributes()
    {
        x = *(float*)(base_address + rel_offsets.x);
        y = *(float*)(base_address + rel_offsets.y);
        z = *(float*)(base_address + rel_offsets.z);
    }
};

struct dynamic_ent : entity
{
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

    void resolve_attributes()
    {
        health = *(__uint64_t*)(base_address+rel_offsets.health);
        armor = *(__uint64_t*)(base_address + rel_offsets.armor);
        rifle_ammo = *(__uint64_t*)(base_address + rel_offsets.rifle_ammo);
        pistol_ammo = *(__uint64_t*)(base_address + rel_offsets.pistol_ammo);
        grenades = *(__uint64_t*)(base_address + rel_offsets.grenades);
        pistol_ammo_reserve = *(__uint64_t*)(base_address + rel_offsets.pistol_ammo_reserve);
        rifle_ammo_reserve = *(__uint64_t*)(base_address + rel_offsets.rifle_ammo);
        x = *(float*)(base_address + rel_offsets.x);
        y = *(float*)(base_address + rel_offsets.y);
        z = *(float*)(base_address + rel_offsets.z);
        yaw = *(float*)(base_address + rel_offsets.yaw);
        pitch = *(float*)(base_address + rel_offsets.pitch);
    }
};

// this is the gamestate we share with the reinforcement learning environment
struct Features
{
    traceresult_s rays[4];
    dynamic_ent *player1;
    std::vector<dynamic_ent*> dynamic_entities;
    std::vector<static_ent*> static_entities;
};

class Feature_Resolver
{
public:
    Features *features;

    void (*TraceLine)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr);

    // resolve player entity features including player1
    void Resolve_Dynamic_Entities();
    void Resolve_Static_Entities();
    // traces rays from player1
    void Ray_Trace(float yaw_offset, float pitch_offset);

    Feature_Resolver(dynamic_ent *p, Features *f)
    {
        features = f;
        features->player1 = p;
    };
};