#include "ac_detour.h"

// implement the AC_detour class

__attribute__((naked)) void AC_detour::trampoline_function()
{
    // execute stolen instructions
    __asm__(
        "mov %fs:0x28,%rax;"
        "mov %rax,0x1a0(%rsp);"
    );
    __uint64_t hook_function_address = (__uint64_t)&AC_detour::hook_function; 
    __asm__(
        "call *%0;"
        :
        : "r" (hook_function_address)
    );

    // return controll flow to the game
    // fix .. attempting to access class variables in a static method
    __uint64_t return_address = check_input_address+injection_offset+hook_instruction_length;
    __asm__(
        "mov %0, %%rdx;"
        "jmp *%%rdx;"
        :
        : "r" (return_address)
        : "%rdx"
    );
}

// super evil code >:)
void AC_detour::hook_function()
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

__uint64_t AC_detour::find_target_page()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    // get proc mappings
    pid_t pid = getpid();
    std::ostringstream mapping;
    mapping << "/proc/" << pid << "/maps";
    std::string proc_mapping_path = mapping.str();

    std::ifstream file(proc_mapping_path);

    if (!file.is_open()) 
    {
        outFile << "failed to open proc mapping\n";
    }
    outFile << "file is open\n";

    // locate the address space we are manipulating
    std::string line;
    for (int i = 0; i < 2; i++)
    {
        std::getline(file,line);
    }

    file.close();

    outFile << line << "\n";

    std::string page_substr = line.substr(0,12);
    page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));

    return page_number;
}

__uint8_t *AC_detour::formulate_detour_instructions()
{
    // fix
    __uint64_t trampoline_function_address = (__uint64_t)&trampoline_function;
    __uint8_t* hook_instruction = new __uint8_t[hook_instruction_length];
    
    hook_instruction[0] = 0x48;
    hook_instruction[1] = 0xBA;
    std::memcpy(hook_instruction+2, &trampoline_function_address, sizeof(trampoline_function_address));
    hook_instruction[10] = 0xFF;
    hook_instruction[11] = 0xE2;
    
    for (int i = 12; i < hook_instruction_length; i++) {
        hook_instruction[i] = 0x90;
    }

    return hook_instruction;  
}

void AC_detour::inject_detour_instructions(__uint8_t *hook_instruction, void *hook_location)
{
    if (mprotect((void*)page_number, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        perror("mprotect error");
        return; 
    }
    std::memcpy(hook_location, hook_instruction, hook_instruction_length);  // hook

    mprotect((void*)page_number, page_size, PROT_READ | PROT_EXEC);
}

void AC_detour::execute_hook()
{
    check_input_address = find_target_page() + check_input_offset;
    void *hook_location = (void*)(check_input_address+injection_offset);
    
    __uint8_t *hook_instruction = formulate_detour_instructions();

    inject_detour_instructions(hook_instruction, hook_location);
}