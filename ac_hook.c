#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>
//void _Z8_setskinii(void *, int skin, int team);

__attribute__((constructor)) void init() {
    FILE *fp = fopen("/home/jacob/UB/cse368/cse-368-team-project/hook_log","a");
    
    //fprintf(fp, "set skin %p\n", &_Z8_setskinii);
    pid_t pid = getpid();
    char buffer[256];
    sprintf(buffer, "/proc/%d/maps", pid);
    FILE *pm_fp = fopen(buffer,"r");
    
    char line_buffer[256];
    int i = 0;
    while(fgets(line_buffer, sizeof(line_buffer), pm_fp)) {
        i ++;
        if (i == 2) {
            break;
        }
    }
    
    char parse_buffer[16];
    for (int i=0; i < 16; i++) {
        if(line_buffer[i] == '-') {
            break;
        } else {
            parse_buffer[i] = line_buffer[i];
            parse_buffer[i+1] = '\0';
        }
    }
    
    fprintf(fp, "parse buffer %s\n", parse_buffer);

    __uint64_t page = strtoull(parse_buffer,NULL,16);
    __uint64_t set_skin_offset = 0x2E5F0;
    __uint64_t set_skin = page + set_skin_offset;

    fprintf(fp, "set skin address %lx\n", set_skin);

    __uint64_t check_input_offset = 0x69B00;
    __uint64_t check_input = page + check_input_offset; // detour this address

    fprintf(fp, "check input address %lx\n", check_input);

    __uint32_t player_offset = *(__uint32_t*)(set_skin + 0x6);
    fprintf(fp, "player offset %x\n", player_offset);
    
    __uint64_t player_entity = *(__uint64_t*)(set_skin + 0xa + player_offset);
    fprintf(fp, "player entity %lx\n", player_entity);

    __uint64_t player_health = *(__uint64_t*)(player_entity + 0x100);
    fprintf(fp, "health %lu\n", player_health);

    *(__uint64_t*)(player_entity + 0x100) = 1000; // overwrite player health

    fclose(fp);
}