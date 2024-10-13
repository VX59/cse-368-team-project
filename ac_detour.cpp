#include "ac_detour.h"

// implement the AC_detour class

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

__uint8_t *AC_detour::formulate_detour_instructions(__uint64_t trampoline_function_address)
{
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

void AC_detour::execute_hook(__uint64_t trampoline_function_address)
{
    check_input_address = find_target_page() + check_input_offset;
    
    __uint8_t *hook_instruction = formulate_detour_instructions(trampoline_function_address);
    
    void *hook_location = (void*)(check_input_address+injection_offset);

    inject_detour_instructions(hook_instruction, hook_location);
}