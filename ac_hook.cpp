#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <random>

__uint64_t check_input_address = NULL;
__uint64_t page_number = NULL;

const __uint64_t injection_offset = 17;
const __uint8_t hook_instruction_length = 17;
const __uint64_t page_size = 0x13D000;

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
    __uint64_t return_address = check_input_address+injection_offset+hook_instruction_length;
    __asm__(
        "mov %0, %%rdx;"
        "jmp *%%rdx;"
        :
        : "r" (return_address)
        : "%rdx"
    );
}           

void detour(void *hook_location, __uint8_t *hook_instruction) 
{
    if (mprotect((void*)page_number, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        perror("mprotect error");
        return; 
    }
    std::memcpy(hook_location, hook_instruction, hook_instruction_length);  // hook

    mprotect((void*)page_number, page_size, PROT_READ | PROT_EXEC);
}

__uint64_t find_target_page() {
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

__uint8_t *formulate_detour_instructions() {
    
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

void __attribute__((constructor)) init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");
    __uint64_t page_number = find_target_page();

    __uint64_t check_input_offset = 0x69B00;

    check_input_address = page_number + check_input_offset;

    __uint8_t *hook_instruction = formulate_detour_instructions();
    void *hook_location = (void*)(check_input_address+injection_offset);
    
    detour(hook_location, hook_instruction);

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