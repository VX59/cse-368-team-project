#include <cstdio>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;


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

        std::string test_message = "Hello, server!\n"; // this would get echod back
        boost::asio::write(socket, boost::asio::buffer(test_message), ec);

        if (ec) 
        {
            std::cerr << "Error attempting to write: " << ec.message() << std::endl;
        }

        // receive response

        char reply[128];
        size_t reply_length = boost::asio::read(socket, boost::asio::buffer(reply, test_message.size()),ec);
        
        if (ec) 
        {
            std::cerr << "Error attempting to write: " << ec.message() << std::endl;
        }

        std::cout.write(reply, reply_length);
    } catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    

    return 0;   
}