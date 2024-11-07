#include "ac_detour.h"

// injector script shall dynamically change these
__uint64_t AC_detour::hook_injection_offset = 17;
__uint8_t AC_detour::hook_instruction_length = 17;

void AC_detour::find_target_page() {
    // opening memory map for game
    std::ostringstream mapping;
    mapping << "/proc/" << getpid() << "/maps";
    std::ifstream mapFile(mapping.str());

    if (!mapFile.is_open()) 
    {
        return;
    }

    // grabbing 2nd line which will be executable memory
    std::string line;
    for (int i = 0; i < 2; i++)
    {
        std::getline(mapFile,line);
    }
    mapFile.close();

    // gets executable page address from 2nd line then sets hook victim address
    std::string page_substr = line.substr(0,12);    
    executable_page_address = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));
}

void AC_detour::formulate_detour_instructions() {
    hook_instructions = new __uint8_t[AC_detour::hook_instruction_length];

    /*
        MOV rdx, trampoline_function_address
        JMP rdx
    */
    hook_instructions[0] = 0x48;
    hook_instructions[1] = 0xBA;
    std::memcpy(hook_instructions + 2, &trampoline_function_address, sizeof(trampoline_function_address));
    hook_instructions[10] = 0xFF;
    hook_instructions[11] = 0xE2;

    // NOP padding
    for (int i = 12; i < AC_detour::hook_instruction_length; i++) {
        hook_instructions[i] = 0x90;
    }
}

void AC_detour::inject_detour_instructions() 
{
    original_instructions = mmap(NULL, AC_detour::hook_instruction_length + 2, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (original_instructions == MAP_FAILED) {
        perror("mmap error");
        return;
    }

    // copying original instructions to executable memory space alongside a
    // JMP RDX instruction
    std::memcpy(original_instructions, hook_victim_address, AC_detour::hook_instruction_length);
    reinterpret_cast<__uint8_t*>(original_instructions)[AC_detour::hook_instruction_length] = 0xFF;
    reinterpret_cast<__uint8_t*>(original_instructions)[AC_detour::hook_instruction_length + 1] = 0xE2;

    // allow writing to executable page, injecting hook, then removing given
    // write permissions
    if (mprotect((void*)executable_page_address, AC_detour::target_page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        perror("mprotect error");
        return; 
    }
    std::memcpy(hook_victim_address, hook_instructions, AC_detour::hook_instruction_length);
    mprotect((void*)executable_page_address, AC_detour::target_page_size, PROT_READ | PROT_EXEC);
}

AC_detour::AC_detour() { }
AC_detour::AC_detour(__uint64_t hook_vict_address, __uint64_t trampoline_function_addr)
{
    hook_victim_address = (void *)(hook_vict_address + hook_injection_offset);
    trampoline_function_address = trampoline_function_addr;

    // first we have to find the executable page address (where game functions
    // are stored), then create our detour instructions and finally inject them
    find_target_page();
    formulate_detour_instructions();
    inject_detour_instructions();
}
