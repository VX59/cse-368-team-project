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
    /**
     * Address of function to "trampoline" to from hook instructions.
     */
    __uint64_t trampoline_function_address;

    /**
     * The hooking instructions to be injected.
     */
    __uint8_t *hook_instructions;

    void find_target_page();
    void formulate_detour_instructions();
    void inject_detour_instructions();

public:
    /**
     * Page size of our target page, AKA the executable page memory map.
     */
    static const __uint64_t target_page_size = 0x13D000;

    /**
     * The offset within hook victim where injection begins.
     * 
     * NOTE: This is expected to be set dynamically by accompanying GDB script,
     *       but is also hardcoded in-case.
     */
    static __uint64_t hook_injection_offset;

    /**
     * The length of the hooking instructions that will be injected.
     * 
     * 
     * NOTE: This is expected to be set dynamically by accompanying GDB script,
     *       but is also hardcoded in-case.
     */
    static __uint8_t hook_instruction_length;

    /**
     * The address of page that is executable within game.
     */
    __uint64_t executable_page_address;

    /**
     * The direct address where hook instructions will be injected
     */
    void *hook_victim_address;

    /**
     * Pointer to memory map where original instructions (plus a jump to RDX)
     * are stored.
     */
    void *original_instructions;

    AC_detour();
    AC_detour(__uint64_t hook_victim_addr, __uint64_t trampoline_function_addr);
};
