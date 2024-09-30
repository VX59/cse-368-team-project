#include "hack_util.h"
int main() {
    try 
    {
        boost::asio::io_context io_context;
        
        // create a resolver from an io_context
        tcp::resolver resolver(io_context); 

        // query for the resolver
        tcp::resolver::query query("localhost","55555");    
        boost::system::error_code ec;

        // resolve the query to a list of enpoints
        tcp::resolver::results_type endpoints = resolver.resolve(query, ec);  

        if (ec) 
        {
            std::cerr << "Error during resolve: " << ec.message() << std::endl;
            return 1;
        }

        tcp::socket socket(io_context);
        // connects to the first available endpoint from the resolver
        boost::asio::connect(socket, endpoints, ec);

        if (ec) 
        {
            std::cerr << "Error attempting to connect: " << ec.message() << std::endl;
            return 1;
        }

        Message msg;
        msg.sender = "Jacob";
        msg.receiver = "server";
        msg.type = 13;
        memset(msg.data, 0, sizeof(msg.data));

        std::ostringstream obuffer;
        msg.serialize(obuffer);
        std::string test_message = obuffer.str();
        size_t test_message_size = test_message.size();
        // send the length of serialized message
        boost::asio::write(socket, boost::asio::buffer(&test_message_size, sizeof(test_message_size)));
        boost::asio::write(socket, boost::asio::buffer(test_message), ec);

        if (ec) {
            std::cerr << "Error attempting to write: " << ec.message() << std::endl;
        } else {
            std::cout << "Message sent successfully.\n";
        }

        // receive response
        // first read the length of incoming serialized data
        size_t response_size;
        boost::asio::read(socket, boost::asio::buffer(&response_size, sizeof(response_size)), ec);
        if (ec) {
            std::cerr << "Error reading message size: " << ec.message() << std::endl;
            return 1;
        }

        std::vector<char> reply(response_size);
            size_t reply_length = boost::asio::read(socket, boost::asio::buffer(reply.data(), response_size), ec); // hardcoded poop

        if (ec) {
            std::cerr << "Error attempting to read: " << ec.message() << std::endl;
        } else {
            std::cout << "Received response of length: " << reply_length << std::endl;
        }

        std::istringstream iss(std::string(reply.begin(), reply.end()));
        Message response;        
        response.deserialize(iss);

        std::cout << "sender: " << response.sender << '\n';
        std::cout << "receiver: " << response.receiver << '\n';
        std::cout << "type: " << response.type << '\n';

        printf("data contents: ");
        for (size_t i = 0; i < sizeof(__uint64_t); i++) {
            printf("%02X ", (unsigned char)response.data[i]);
        }
        printf("\n");
        std::cout << std::endl;
    } catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}
