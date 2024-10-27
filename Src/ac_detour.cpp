#include "ac_detour.h"

__uint64_t AC_detour::injection_offset = 17;
__uint8_t AC_detour::hook_instruction_length = 17;

void AC_detour::find_target_page() {
    pid_t pid = getpid();
    std::ostringstream mapping;
    mapping << "/proc/" << pid << "/maps";
    std::string proc_mapping_path = mapping.str();

    std::ifstream file(proc_mapping_path);

    if (!file.is_open()) 
    {
        return;
    }

    std::string line;
    for (int i = 0; i < 2; i++)
    {
        std::getline(file,line);
    }

    file.close();

    std::string page_substr = line.substr(0,12);    

    page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));
    check_input_address = page_number + check_input_offset;
}

void AC_detour::formulate_detour_instructions() {
    
    hook_instruction = new __uint8_t[AC_detour::hook_instruction_length];
    
    /*
    mov rdx, trampoline_function_address
    jmp rdx
    nop... 
     */

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
    original_instructions = mmap(nullptr, AC_detour::hook_instruction_length+2, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (original_instructions == MAP_FAILED) {
        perror("mmap error");
        return;
    }
    hook_location = (void*)(check_input_address+injection_offset);

    std::memcpy(original_instructions, hook_location, AC_detour::hook_instruction_length);
    // JMP RDX
    reinterpret_cast<__uint8_t*>(original_instructions)[AC_detour::hook_instruction_length] = 0xFF;
    reinterpret_cast<__uint8_t*>(original_instructions)[AC_detour::hook_instruction_length+1] = 0xE2;

    if (mprotect((void*)page_number, AC_detour::target_page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        perror("mprotect error");
        return; 
    }
    std::memcpy(hook_location, hook_instruction, AC_detour::hook_instruction_length); // hook

    mprotect((void*)page_number, AC_detour::target_page_size, PROT_READ | PROT_EXEC);
}

AC_detour::AC_detour(__uint64_t trampoline_function_addr)
{
    trampoline_function_address = trampoline_function_addr;
    find_target_page();
    formulate_detour_instructions();
    inject_detour_instructions();
}