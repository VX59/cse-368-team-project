#
# This script is meant to be run by the GDB process inside fellow script
# 'injector.sh', it dynamically sets otherwise hardcoded values like
# function addresses and instruction patch sizes.
#
# Not using 'injector.sh' (just 'ac_detour.so') may not work on all AC exes.
#

import gdb

instruction_patch_idx = str(int(gdb.parse_and_eval('AC_detour::hook_injection_offset')))
instruction_patch_size = str(int(gdb.parse_and_eval('AC_detour::hook_instruction_length')))

###############################################################################
#
# Setting addresses of functions we want
#
gdb.execute('set AC_function_addresses.checkinput = &checkinput')
gdb.execute('set AC_function_addresses.raycube = &raycube')
gdb.execute('set AC_function_addresses.TraceLine = &TraceLine')
gdb.execute('set AC_function_addresses._setskin = &_setskin')
gdb.execute('set AC_function_addresses.calcteamscores = &calcteamscores')
gdb.execute('set AC_function_addresses.mapmodelslotusage = &mapmodelslotusage')

###############################################################################
#
# Setting addresses of symbols we want
#
gdb.execute('set AC_symbol_addresses.player1 = &player1')
gdb.execute('set AC_symbol_addresses.players = &players')
gdb.execute('set AC_symbol_addresses.ents = &ents')
gdb.execute('set AC_symbol_addresses.screenw = &screenw')
gdb.execute('set AC_symbol_addresses.screenh = &screenh')
gdb.execute('set AC_symbol_addresses.mvpmatrix = &mvpmatrix')

###############################################################################
#
# Extracting the instructions that will get patched out, via GDB output, so we
# can modify the default hook instruction length
#
def get_instruction_line_addr(line):
    addr_idx = line.find("0x")
    addr_end_idx = line.find("<") - 1
    try: return int(line[addr_idx:addr_end_idx], 16)
    except: return 0

def get_instruction(line):
    addr_idx = line.find(">:\t") + 3
    addr_end_idx = len(line)
    try: return line[addr_idx:addr_end_idx]
    except: return None

# Basically will go through instructions until we hit instruction that will not
# be overwritten by trampoline jump
prev_addr = None
instructions = []
instruction_sizes = 0

for line in gdb.execute('x/%si (&checkinput+%s)' % (instruction_patch_size, instruction_patch_idx), to_string=True).splitlines():
    instructions.append(' '.join(get_instruction(line).split()))
    if prev_addr == None:
        prev_addr = get_instruction_line_addr(line)
        continue
    instr_addr = get_instruction_line_addr(line)
    cur_size = instr_addr - prev_addr
    prev_addr = instr_addr
    instruction_sizes += cur_size

    # We have the instructions that will be patched out
    if instruction_sizes >= int(instruction_patch_size):
        instructions = instructions[:-1]
        break

# Our new hook instruction length should be the length of FULL instruction
# bytes we overwrite
gdb.execute('set AC_detour::hook_instruction_length = %d' % instruction_sizes)
