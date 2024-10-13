#include "ac_detour.h"

__uint64_t AC_detour::find_target_page() {
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
    __uint64_t page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));
    outFile.close();
    return page_number;
}