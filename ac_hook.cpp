#include "hack_util.h"
#include "cstring"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>

void hook_function() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    outFile << "looks like there was a detour hehe\n";
    outFile.close();        
}

__uint64_t check_input = 0;
__uint8_t *original_func_prologue = nullptr;
__uint64_t page_number = 0;
__uint8_t jump_bytes = 16;
__uint64_t page_size = 0x13D000;

__attribute__((constructor)) void init()
{

    original_func_prologue = new __uint8_t[jump_bytes];

    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    std::ostringstream message;
    // get proc mappings
    pid_t pid = getpid();
    std::ostringstream mapping;
    mapping << "/proc/" << pid << "/maps";
    std::string proc_mapping_path = mapping.str();
    outFile << "proc mapping path " << proc_mapping_path << "\n";

    std::ifstream file(proc_mapping_path);

    if (!file.is_open()) 
    {
        outFile << "failed to open proc mapping\n";
    }
    outFile << "file is open\n";

    std::string line;
    for (int i = 0; i < 2; i++) {
        std::getline(file,line);
    }

    file.close();

    outFile << line << "\n";

    std::string page_substr = line.substr(0,12);
    page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));

    __uint64_t check_input_offset = 0x69B00;
    check_input = page_number + check_input_offset; // detour this address
    outFile << "check input address " << "0x" << std::hex << std::uppercase << check_input  << "\n";

    __uint64_t hook_func_addr = (__uint64_t)&hook_function;
    __uint8_t* hook_instruction = new __uint8_t[jump_bytes];

    std::memset(hook_instruction, 0, jump_bytes);

    hook_instruction[0] = 0x48; // REX.W prefix 64-bit operand size
    hook_instruction[1] = 0xB8;  // mov rax, imm64
    std::memcpy(hook_instruction+2, &hook_func_addr, sizeof(hook_func_addr));
    hook_instruction[10] = 0xFF; // jmp rax
    hook_instruction[11] = 0xE0; // opcode ext for jmp rax
    
    for (int i = 12; i < 16; i++) {
        hook_instruction[i] = 0x0;
    }

    outFile << std::hex << (int)hook_func_addr << std::endl;

    // hook instruction bytes
    for (int i = 0; i < jump_bytes; i++) {
        outFile << std::hex << (int)hook_instruction[i];
        outFile << " ";
    }
    outFile << std::endl;

    // problem ?    
    
    std::memcpy(original_func_prologue,(void *)check_input, jump_bytes);     // copy the original 12 bytes to save for recovery .. segfaulting

    // check input original 16 bytes
    for (int i = 0; i < jump_bytes; i++) {
        outFile << std::hex << (int)(*(__uint8_t*)(check_input + i)) << " ";
    }
    outFile << std::endl;

    if (mprotect((void*)page_number, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        perror("mprotect");
        outFile << "mprotect failed\n";
        return; 
    }

    outFile << "change protections on " << std::hex << page_number << std::endl;
    
    std::ifstream file_again(proc_mapping_path);

    if (!file_again.is_open()) 
    {
        outFile << "failed to open proc mapping\n";
    }

    outFile << "file is open\n";

    for (int i = 0; i < 2; i++) {
        std::getline(file_again,line);
    }

    file_again.close();
    outFile << line << "\n";

    outFile << std::hex << (void *)check_input << " " << std::hex << (__uint64_t)hook_instruction << " " << (__uint64_t)jump_bytes << std::endl;
            std::memcpy((void*)check_input, hook_instruction, jump_bytes);  // overwrite the first 12 bytes of checkinput to jump

    // check input original 16 bytes
    for (int i = 0; i < jump_bytes; i++) {
        outFile << std::hex << (int)(*(__uint8_t*)(check_input + i)) << " ";
    }
    outFile << std::endl;

    mprotect((void*)page_number, page_size, PROT_READ | PROT_EXEC);
    outFile.close();
}

__attribute__((destructor)) void unload() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    mprotect((void*)page_number, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

    //std::memcpy((void*)check_input, original_func_prologue, jump_bytes);
    
    mprotect((void*)page_number, page_size, PROT_READ | PROT_EXEC);
    
    outFile << "successfully unhooked\n";
    outFile.close();

}