#include <unistd.h>

class player_ent
{
public:
    __uint64_t base_addresss;
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

    player_ent(__uint64_t base_address)
    {
        base_addresss = base_address;
    }

    void resolve_attributes()
    {
        health = *(__uint64_t*)(base_addresss+0x100);
        armor = *(__uint64_t*)(base_addresss + 0x104);
        rifle_ammo = *(__uint64_t*)(base_addresss + 0x154);
        pistol_ammo = *(__uint64_t*)(base_addresss + 0x140);
        grenades = *(__uint64_t*)(base_addresss + 0x152);
        pistol_ammo_reserve = *(__uint64_t*)(base_addresss + 0x11C);
        rifle_ammo_reserve = *(__uint64_t*)(base_addresss + 0x130);
        x = *(float*)(base_addresss + 0x2c);
        y = *(float*)(base_addresss + 0x30);
        z = *(float*)(base_addresss + 0x34);
        yaw = *(float*)(base_addresss + 0x38);
        pitch = *(float*)(base_addresss + 0x3c);
    }
};
