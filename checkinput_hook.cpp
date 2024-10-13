#include "ac_detour.h"        

AC_detour *detour = new AC_detour();

// super evil code >:)
void hook_function()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    // generate a random number so i can see the log file changing
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(1, 1000);

    int x = distr(gen);

    outFile << "looks like there is a detour here hehe " << x;
    outFile.close();  
}

__attribute__((naked)) void trampoline_function()
{
    // execute stolen instructions
    __asm__(
        "mov %fs:0x28,%rax;"
        "mov %rax,0x1a0(%rsp);"
    );
    __uint64_t hook_function_address = (__uint64_t)&hook_function;
    __asm__(
        "call *%0;"
        :
        : "r" (hook_function_address)
    );

    // return controll flow to the game
    // fix .. attempting to access class variables in a static method
    __uint64_t return_address = detour->check_input_address+detour->injection_offset+detour->hook_instruction_length;
    __asm__(
        "mov %0, %%rdx;"
        "jmp *%%rdx;"
        :
        : "r" (return_address)
        : "%rdx"
    );
}

void __attribute__((constructor)) init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    detour->execute_hook((__uint64_t)&trampoline_function);

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