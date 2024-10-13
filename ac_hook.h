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
    uint64_t check_input_address;
    uint64_t injection_site;
    uint64_t page_number;

    const uint64_t injection_offset = 17;
    const uint8_t hook_instruction_length = 17;
    const uint64_t page_size = 0x13D000;


    void __attribute__((naked)) trampoline_function();

    static void hook_function();

    uint64_t find_target_page();

    uint8_t *formulate_detour_instructions();

    void inject_detour_instructions(__uint8_t *hook_instruction);
public:
    void operator()();
};