#include "hack_util.h"
#include "cstring"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>

__uint8_t *original_func_prologue = nullptr;
__uint64_t check_input = NULL;
__uint64_t page_number = NULL;
__uint8_t jump_bytes = 16;
__uint64_t page_size = 0x13D000;

void hook_function()
{
    //std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log", std::ofstream::app);
    
    //outFile << "looks like there was a detour hehe\n";
    //outFile.close();

    // void (*original_func)(void) = (void (*) (void))(check_input+16);
    // __asm__( "push %rbp;"
    //          "push %r15;"
    //          "push %r14;"
    //          "push %r13;"
    //          "push %r12;"
    //          "push %rbx;");
    // original_func();

// execute first 16 bytes of original function
// execute the rest of original function starting from handle+17
}

void detour_hook(void *target_func, __uint8_t *hook_instruction) 
{
    std::memcpy(original_func_prologue,target_func, jump_bytes);     // save original instructions
    if (mprotect((void*)page_number, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        perror("mprotect");
        return; 
    }
    std::memcpy((void*)(check_input+17), hook_instruction, jump_bytes);  // hook
    mprotect((void*)page_number, page_size, PROT_READ | PROT_EXEC);
}

// jump here
void __attribute__((naked)) trampoline_function()
{
    // jump to the detour hook from here after we store the original asm code
    __asm__("push %rbp;"
            "push %r15;"
            "push %r14;"
            "push %r13;"
            "push %r12;"
            "push %rbx;"
            "sub $0x1a8,%rsp;"
            );

    __uint64_t hook_function_addr = (__uint64_t)&hook_function;
    __uint64_t target_jump_addr = check_input+16;

    __asm__(
            "call *%0;"       // jumps to hook function
            "jmp *%1;"        // jumps back to check input
            :
            : "r" (hook_function_addr), "r" (target_jump_addr)
    );
}

void __attribute__((constructor)) init()
{

    original_func_prologue = new __uint8_t[jump_bytes];

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

    __uint64_t check_input_offset = 0x69B00;
    check_input = page_number + check_input_offset; // detour this address
    outFile << "check input address " << "0x" << std::hex << std::uppercase << check_input  << "\n";

    __uint64_t trampoline_func_adder = (__uint64_t)&trampoline_function;
    __uint8_t* hook_instruction = new __uint8_t[jump_bytes];

    std::memset(hook_instruction, 0, jump_bytes);
    for (int i = 0; i < 10; i++) {
        hook_instruction[i] = 0x90;
    }

    // hook_instruction[0] = 0x48; // REX.W prefix 64-bit operand size
    // hook_instruction[1] = 0xB8;  // mov rax, imm64
    // std::memcpy(hook_instruction+2, &trampoline_func_adder, sizeof(trampoline_func_adder));
    // hook_instruction[10] = 0xFF; // jmp rax
    // hook_instruction[11] = 0xE0; // opcode ext for jmp rax
    
    // for (int i = 12; i < 16; i++) {
    //     hook_instruction[i] = 0x90;
    // }

    outFile << std::hex << (int)trampoline_func_adder << std::endl;

    // hook instruction bytes
    for (int i = 0; i < jump_bytes; i++)
    {
        outFile << std::hex << (int)hook_instruction[i];
        outFile << " ";
    }

    outFile << std::endl;

    detour_hook((void *)check_input, hook_instruction);

    outFile << "successfully hooked";
    outFile.close();
}

void __attribute__((destructor)) unload()
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/ac_detour.log");

    mprotect((void*)page_number, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

    std::memcpy((void*)check_input, original_func_prologue, jump_bytes);
    
    mprotect((void*)page_number, page_size, PROT_READ | PROT_EXEC);
    
    outFile << "successfully unhooked\n";
    outFile.close();
}