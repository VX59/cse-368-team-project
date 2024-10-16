#include "hack_util.h"

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp::socket socket, std::string response_message)
{
    try
    {
        char data[1024];
        for (;;)
        {
            std::size_t message_length = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);

            // send response
            size_t response_size = response_message.size();
            co_await async_write(socket, boost::asio::buffer(&response_size, sizeof(response_size)));
            co_await async_write(socket, boost::asio::buffer(response_message), use_awaitable);
        }
    }
    catch (std::exception& e)
    {
        std::printf("echo Exception: %s\n", e.what());
    }
}

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

awaitable<void> listener(std::string response_message)
{
    auto executor = co_await this_coro::executor;
    tcp::acceptor acceptor(executor, {tcp::v4(), 55555});
    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(executor, echo(std::move(socket), response_message), detached);
    }
}

__attribute__((constructor)) void init() 
{
    std::ofstream outFile("/home/jacob/UB/cse368/cse-368-team-project/hook_log");
    
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
    std::ostringstream message;
    message << line << "\n";

    std::string page_substr = line.substr(0,12);
    __uint64_t page_number = static_cast<__uint64_t>(std::strtoull(page_substr.c_str(),nullptr, 16));

    message << "page "<< "0x" << std::hex << std::uppercase << page_number << "\n"; 
        
    __uint64_t set_skin_offset = 0x2E5F0;
    __uint64_t set_skin = page_number + set_skin_offset;

    message << "set skin address "<< "0x" << std::hex << std::uppercase << set_skin << "\n";

    __uint64_t check_input_offset = 0x69B00;
    __uint64_t check_input = page_number + check_input_offset; // detour this address
   
    message << "check input address " << "0x" << std::hex << std::uppercase << check_input  << "\n";

    __uint32_t player_ip_offset = *(__uint32_t*)(set_skin + 0x6);   // offset of the playerid from the instruction pointer
    message << "player offset " << "0x" << std::hex << std::uppercase  << player_ip_offset << "\n";

    __uint64_t player_entity = *(__uint64_t*)(set_skin + 0xa + player_ip_offset);
    message << "player entity " << "0x" << std::hex << std::uppercase << player_entity << "\n";

    *(__uint64_t*)(player_entity + 0x100) = 1000; // overwrite player health

    __uint64_t player_health = *(__uint64_t*)(player_entity + 0x100);
    message << "player health " << player_health << "\n";
    
    outFile << message.str();
    outFile.close();

    try
    {
        std::thread server_thread([player_health]()
        {
            boost::asio::io_context io_context(1);

            boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto){ io_context.stop(); });

            Message rpns;
            rpns.sender = "server";
            rpns.receiver = "jacob";
            rpns.type = 24;
            memset(rpns.data, 0, sizeof(rpns.data));
            memcpy(rpns.data,&player_health, sizeof(player_health));

            std::ostringstream obuffer;
            rpns.serialize(obuffer);
            std::string response = obuffer.str();

            co_spawn(io_context, listener(response), detached);

            io_context.run();
        });
        server_thread.detach();
    }
    catch (std::exception& e)
    {
        std::printf("Exception: %s\n", e.what());
    }
}