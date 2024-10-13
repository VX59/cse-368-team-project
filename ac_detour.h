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
    __uint64_t check_input_address;
    __uint64_t page_number;

    const __uint64_t check_input_offset = 0x69B00;
    const __uint64_t injection_offset = 17;
    const __uint8_t hook_instruction_length = 17;
    const __uint64_t page_size = 0x13D000;

    static void __attribute__((naked)) trampoline_function();

    static void hook_function();

    __uint64_t find_target_page();

    __uint8_t *formulate_detour_instructions();

    void inject_detour_instructions(__uint8_t *hook_instruction, void *hook_location);
public:
    void execute_hook();
};