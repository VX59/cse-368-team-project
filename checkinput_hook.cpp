#include "ac_detour.h"

struct hook_util {
    __uint64_t check_input_address;
    __uint64_t page_number;
    __uint8_t *original_instructions;
} (hook_util);

// super evil code >:)
void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    // generate a random number so i can see the log file changing
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(1, 1000);

    int x = distr(gen);

    outFile << "looks like there is a detour here hehe " << x;
    outFile.close();
}

void __attribute__((naked)) trampoline_function()
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
    __uint64_t return_address = hook_util.check_input_address+AC_detour::injection_offset+AC_detour::hook_instruction_length;
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

    AC_detour detour((__uint64_t)&trampoline_function);
    
    hook_util.check_input_address = detour.check_input_address;
    hook_util.page_number = detour.page_number;
    hook_util.original_instructions = detour.original_instructions;

    outFile << "successfully hooked";
    outFile.close();
}

void __attribute__((destructor)) unload()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    
    mprotect((void*)hook_util.page_number, AC_detour::page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    std::memcpy((void*)(hook_util.check_input_address+AC_detour::injection_offset), (void*)hook_util.original_instructions, AC_detour::hook_instruction_length);
    mprotect((void*)hook_util.page_number, AC_detour::page_size, PROT_READ | PROT_EXEC);
    
    outFile << "successfully unhooked\n";
    outFile.close();
}