#include "ac_detour.h"        

void __attribute__((constructor)) init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    
    AC_detour *detour = new AC_detour();
    detour->execute_hook();

    outFile << "successfully hooked";
    outFile.close();
}

void __attribute__((destructor)) unload()
{
    // // fix this ..
    // std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    // mprotect((void*)page_number, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

    // std::memcpy((void*)check_input_address, original_func_prologue, hook_instruction_length);
    
    // mprotect((void*)page_number, page_size, PROT_READ | PROT_EXEC);
    
    // outFile << "successfully unhooked\n";
    // outFile.close();
}