#include "hack_util.h"
#include "cstring"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>

void hook_function(std::ostringstream message) {
    message << "looks like there was a detour hehe";
}

__uint64_t check_input = 0;

unsigned char *original_prolg = (unsigned char*)malloc(12);
__uint64_t page_number = 0;
long page_size = sysconf(_SC_PAGE_SIZE);


__attribute__((constructor)) void init()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/hook_log");
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
    int i = 0;
    int target_page = 2;
    while (std::getline(file, line))
    {
        i++;
        if (i == target_page)
        {
            break;
        }
    }
    message << line << "\n";

    std::string page_substr = line.substr(0,12);
    page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));

    __uint64_t check_input_offset = 0x69B00;
    check_input = page_number + check_input_offset; // detour this address
    message << "check input address " << "0x" << std::hex << std::uppercase << check_input  << "\n";

    __uint64_t hook_func_addr = (__uint64_t)&hook_function;

    unsigned char* hook_instruction = (unsigned char*)malloc(12);
    hook_instruction[0] = 0x48; // REX.W prefix 64-bit operand size
    hook_instruction[1] = 0xB8;  // mov rax, imm64
    std::memcpy(hook_instruction+2, &hook_func_addr, sizeof(hook_func_addr));
    hook_instruction[10] = 0xFF; // jmp rax
    hook_instruction[11] = 0xE0; // opcode ext for jmp rax
    message << hook_instruction << std::endl;
    outFile << message.str();

    // change page protections
    void* aligned_page_num = (void*)((__uint64_t)page_number & ~(page_size-1));
    
    message << std::hex << page_number << " " << std::hex << aligned_page_num << std::endl;
    if (mprotect(aligned_page_num, page_size, PROT_READ | PROT_WRITE) == -1) {
        perror("mprotect");
        message << "mprotect failed\n";
        return;
    }
    message << "change protections on" << std::hex << aligned_page_num << std::endl;
    outFile << message.str();
    std::memcpy((void*)original_prolg,(void*)check_input, 12);     // copy the original 12 bytes to save for recovery
    // //std::memcpy((void*)check_input, (void*)hook_instruction, 12);  // overwrite the first 12 bytes of checkinput to jump
    // mprotect(aligned_page_num, page_size, PROT_READ);
    
    outFile.close();

}

__attribute__((destructor)) void unload() {
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/unhook_log");
    std::ostringstream message;
    void* aligned_page_num = (void*)((__uint64_t)page_number & ~(page_size-1));
    mprotect(aligned_page_num, page_size, PROT_READ | PROT_WRITE);
    std::memcpy((void*)check_input, (void*)original_prolg, 12);
    mprotect(aligned_page_num, page_size, PROT_READ);
    message << "successfully unhooked";
    outFile << message.str();
    outFile.close();

}