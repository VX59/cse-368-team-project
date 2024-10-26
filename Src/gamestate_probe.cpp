#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <iostream>
#include <iomanip>

class player_ent
{
public:
    __uint64_t base_addresss;
    __uint64_t *health;
    __uint64_t *armor;
    __uint64_t *rifle_ammo;
    __uint64_t *pistol_ammo;
    __uint64_t *pistol_ammo_reserve;
    __uint64_t *rifle_ammo_reserve;
    __uint64_t *grenades;
    __uint64_t *x;
    __uint64_t *y;
    __uint64_t *z;
    __uint64_t *yaw;
    __uint64_t *pitch;

    player_ent(__uint64_t base_address)
    {
        base_addresss = base_address;
        health = (__uint64_t*)(base_addresss+0x100);
        armor = (__uint64_t*)(base_addresss + 0x104);
        rifle_ammo = (__uint64_t*)(base_addresss + 0x154);
        pistol_ammo = (__uint64_t*)(base_addresss + 0x140);
        grenades = (__uint64_t*)(base_addresss + 0x152);
        pistol_ammo_reserve = (__uint64_t*)(base_addresss + 0x11C);
        rifle_ammo_reserve = (__uint64_t*)(base_addresss + 0x130);
        x = (__uint64_t*)(base_addresss + 0x2c);
        y = (__uint64_t*)(base_addresss + 0x30);
        z = (__uint64_t*)(base_addresss + 0x34);
        yaw = (__uint64_t*)(base_addresss + 0x38);
        pitch = (__uint64_t*)(base_addresss + 0x3c);
    }
};

__attribute__((constructor)) void init() 
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/hook_log");
    
    pid_t pid = getpid();

    std::ostringstream mapping;
    mapping << "/proc/" << pid << "/maps";
    std::string proc_mapping_path = mapping.str();
    outFile << "proc mapping path " << proc_mapping_path << "\n";

    std::ifstream file(proc_mapping_path);

    if (!file.is_open()) 
    {
        outFile << "failed to open proc mapping\n";
    }
    outFile << "file is open\n";
    
    std::string line;
    int i = 0;
    int target_page = 2;
    while (std::getline(file, line))
    {
        i++;
        if (i == target_page)
        {
            break;
        }
    }
    std::ostringstream message;
    outFile << line << "\n";

    std::string page_substr = line.substr(0,12);
    __uint64_t page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));

    //outFile << "page "<< "0x" << std::hex << std::uppercase << page_number << "\n"; 

    __uint64_t set_skin_offset = 0x2E5F0;
    __uint64_t set_skin = page_number + set_skin_offset;

    //outFile << "set skin address "<< "0x" << std::hex << std::uppercase << set_skin << "\n";
   
    __uint32_t player_ip_offset = *(__uint32_t*)(set_skin + 0x6);   // offset of the playerid from the instruction pointer
    //outFile << "player offset " << "0x" << std::hex << std::uppercase  << player_ip_offset << "\n";

    player_ent player1(*(__uint64_t*)(set_skin + 0xa + player_ip_offset));
    //outFile << "player entity " << "0x" << std::hex << std::uppercase << base_addresss << "\n";

    outFile << "health" << std::hex << player1.health << std::endl;
    
    *player1.health = 1000;

    outFile << "player health " << *player1.health << "\n";
    outFile << "player armor " << *player1.armor << "\n";
    outFile << "player rile ammo " << *player1.rifle_ammo << "\n";
    outFile << "player grenades " << *player1.grenades << "\n";

    outFile.close();
}