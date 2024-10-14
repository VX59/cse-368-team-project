#include "ac_detour.h"

void AC_detour::find_target_page() {
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
    outFile.close();

    std::string page_substr = line.substr(0,12);

    page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));
    check_input_address = page_number + check_input_offset;
}

void AC_detour::formulate_detour_instructions() {
    
    hook_instruction = new __uint8_t[AC_detour::hook_instruction_length];
    
    hook_instruction[0] = 0x48;
    hook_instruction[1] = 0xBA;
    std::memcpy(hook_instruction+2, &trampoline_function_address, sizeof(trampoline_function_address));
    hook_instruction[10] = 0xFF;
    hook_instruction[11] = 0xE2;
    
    for (int i = 12; i < AC_detour::hook_instruction_length; i++) {
        hook_instruction[i] = 0x90;
    }
}

void AC_detour::inject_detour_instructions() 
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

AC_detour::AC_detour(__uint64_t trampoline_function_addr)
{
    trampoline_function_address = trampoline_function_addr;
    find_target_page();
    formulate_detour_instructions();
    hook_location = (void*)(check_input_address+injection_offset);

    inject_detour_instructions();

}