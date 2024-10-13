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
public:
    static const __uint64_t injection_offset = 17;
    static const __uint8_t hook_instruction_length = 17;
    static const __uint64_t page_size = 0x13D000;
    static const __uint64_t check_input_offset = 0x69B00;

    __uint64_t find_target_page();
};