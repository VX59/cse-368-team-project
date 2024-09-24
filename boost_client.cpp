#include <cstdio>
#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

using boost::asio::ip::tcp;

enum opcode {
    GET_GAMESTATE,
    POST_REACTION
};

class Message 
{
public:
    std::string sender;
    std::string receiver;
    int type;
    char data[256];

    void serialize(std::ostringstream& oss) const
    {
        size_t sender_length = sender.size();
        size_t receiver_length = receiver.size();
        oss.write(reinterpret_cast<const char*>(&sender_length), sizeof(sender_length));
        oss.write(sender.c_str(), sender_length);

        oss.write(reinterpret_cast<const char *>(&receiver_length),sizeof(receiver_length));
        oss.write(receiver.c_str(), receiver_length);

        oss.write(reinterpret_cast<const char*>(&type), sizeof(type));
        oss.write(data, sizeof(data));
    }

    void deserialize(std::istringstream& iss)
    {
        size_t sender_length;
        size_t receiver_length;
        size_t type_length;

        iss.read(reinterpret_cast<char*>(&sender_length), sizeof(sender_length));
        sender.resize(sender_length);
        iss.read(&sender[0], sender_length);

        iss.read(reinterpret_cast<char*>(&receiver_length), sizeof(receiver_length));
        receiver.resize(receiver_length);
        iss.read(&receiver[0], receiver_length);

        iss.read(reinterpret_cast<char*>(&type), sizeof(type));
        iss.read(data, sizeof(data));
    }
};

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

        // std::cout << "connected \n";
        // Message msg;
        // msg.sender = "Jacob";
        // msg.receiver = "server";
        // msg.type = 13;
        // memset(msg.data, 0, sizeof(msg.data));

        // std::ostringstream obuffer;
        // msg.serialize(obuffer);

        // std::string test_message = obuffer.str();
        // std::cout << "Serialized message length: " << test_message.size() << std::endl;
        // std::cout << "Serialized message contents: " << std::string(test_message.begin(), test_message.end()) << std::endl;  // Print in binary
        std::string test_message = "hello server";
        boost::asio::write(socket, boost::asio::buffer(test_message.data(),test_message.size()), ec);

        if (ec) {
            std::cerr << "Error attempting to write: " << ec.message() << std::endl;
        } else {
            std::cout << "Message sent successfully.\n";
        }

        // receive response
        char reply[1024];
        size_t reply_length = boost::asio::read(socket, boost::asio::buffer(reply, 17)); // hardcoded poop

        if (ec) {
            std::cerr << "Error attempting to read: " << ec.message() << std::endl;
        } else {
            std::cout << "Received response of length: " << reply_length << std::endl;
        }

        std::cout.write(reply, reply_length);
        std::cout << std::endl;
        // std::istringstream iss(std::string(reply, reply_length));
        // Message response;        
        // response.deserialize(iss);

        // std::cout << "Raw reply data in hex: ";
        // std::cout << std::endl;
        // std::cout << "sender: " << response.sender << '\n';
        // std::cout << "receiver: " << response.receiver << '\n';
        // std::cout << "type: " << std::hexfloat << response.type << '\n';
        // std::cout << "data: " << response.data << '\n';

    } catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }
    

    return 0;   
}