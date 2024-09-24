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

struct player_entity 
{
    __uint64_t health;
} player1;

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
    message << line << "\n";

    std::string page_substr = line.substr(0,12);
    __uint64_t page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));

    message << "page "<< "0x" << std::hex << std::uppercase << page_number << "\n"; 

    __uint64_t set_skin_offset = 0x2E5F0;
    __uint64_t set_skin = page_number + set_skin_offset;

    message << "set skin address "<< "0x" << std::hex << std::uppercase << set_skin << "\n";


    __uint64_t check_input_offset = 0x69B00;
    __uint64_t check_input = page_number + check_input_offset; // detour this address
   
    message << "check input address " << "0x" << std::hex << std::uppercase << check_input  << "\n";

    __uint32_t player_ip_offset = *(__uint32_t*)(set_skin + 0x6);   // offset of the playerid from the instruction pointer
    message << "player offset " << "0x" << std::hex << std::uppercase  << player_ip_offset << "\n";

    __uint64_t player_entity = *(__uint64_t*)(set_skin + 0xa + player_ip_offset);
    message << "player entity " << "0x" << std::hex << std::uppercase << player_entity << "\n";

    __uint64_t player_health = *(__uint64_t*)(player_entity + 0x100);
    message << "player health " << player_health << "\n";

    *(__uint64_t*)(player_entity + 0x100) = 1000; // overwrite player health

    outFile.close();
}