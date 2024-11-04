#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <random>

class AC_detour
{
private:
    __uint64_t trampoline_function_address;
    __uint8_t *hook_instruction;
    void *hook_location;

    void find_target_page();
    void formulate_detour_instructions();
    void inject_detour_instructions();

public:
    static const __uint64_t injection_offset = 17;
    static const __uint8_t hook_instruction_length = 17;
    static const __uint64_t target_page_size = 0x13D000;
    
    __uint64_t victim_offset;
    __uint64_t victim_address;
    __uint64_t page_number;
    __uint8_t *original_instructions;

    AC_detour(__uint64_t vict_offset, __uint64_t trampoline_function_addr);
};