#include "ac_detour.h"

//const __uint64_t injection_offset = 17;
//const __uint8_t hook_instruction_length = 17;
//const __uint64_t page_size = 0x13D000;
//const __uint64_t check_input_offset = 0x69B00;

__uint64_t check_input_address = NULL;
__uint64_t page_number = NULL;
__uint8_t *original_instructions = nullptr;

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
    __uint64_t return_address = check_input_address+AC_detour::injection_offset+AC_detour::hook_instruction_length;
    __asm__(
        "mov %0, %%rdx;"
        "jmp *%%rdx;"
        :
        : "r" (return_address)
        : "%rdx"
    );
}           

void inject_detour_instructions(void *hook_location, __uint8_t *hook_instruction) 
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

     original_instructions = new __uint8_t[AC_detour::hook_instruction_length];

    std::memcpy(original_instructions, hook_location, AC_detour::hook_instruction_length);

    if (mprotect((void*)page_number, AC_detour::page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        perror("mprotect error");
        return; 
    }
    outFile << std::hex << original_instructions << std::hex << std::endl << hook_location << std::endl;
    std::memcpy(hook_location, hook_instruction, AC_detour::hook_instruction_length);  // hook

    mprotect((void*)page_number, AC_detour::page_size, PROT_READ | PROT_EXEC);
    outFile.close();
}

__uint8_t *formulate_detour_instructions() {
    
    __uint64_t trampoline_function_address = (__uint64_t)&trampoline_function;
    __uint8_t* hook_instruction = new __uint8_t[AC_detour::hook_instruction_length];
    
    hook_instruction[0] = 0x48;
    hook_instruction[1] = 0xBA;
    std::memcpy(hook_instruction+2, &trampoline_function_address, sizeof(trampoline_function_address));
    hook_instruction[10] = 0xFF;
    hook_instruction[11] = 0xE2;
    
    for (int i = 12; i < AC_detour::hook_instruction_length; i++) {
        hook_instruction[i] = 0x90;
    }

    return hook_instruction;
}

void execute_hook()
{
    AC_detour detour;
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    page_number = detour.find_target_page();
    check_input_address = page_number + AC_detour::check_input_offset;
    __uint8_t *hook_instruction = formulate_detour_instructions();
    void *hook_location = (void*)(check_input_address+AC_detour::injection_offset);
    
    outFile << "injecting instructions" << std::endl;
    inject_detour_instructions(hook_location, hook_instruction);
    outFile.close();
}

void __attribute__((constructor)) init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    execute_hook();

    outFile << "successfully hooked";
    outFile.close();
}

void __attribute__((destructor)) unload()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    mprotect((void*)page_number, AC_detour::page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

    std::memcpy((void*)(check_input_address+AC_detour::injection_offset), (void*)original_instructions, AC_detour::hook_instruction_length);
    
    mprotect((void*)page_number, AC_detour::page_size, PROT_READ | PROT_EXEC);
    
    outFile << "successfully unhooked\n";
    outFile.close();
}