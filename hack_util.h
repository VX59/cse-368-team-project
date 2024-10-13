#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string>
#include <iomanip>
#include <thread>
#include <cstdio>

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
        oss << sender << '\n' << receiver << '\n' << type << '\n' << data << '\n';
    }

    void deserialize(std::istringstream& iss)
    {
        std::getline(iss, sender);
        std::getline(iss, receiver);
        iss >> type;
        iss.ignore();
        iss.getline(data, sizeof(data));
    }
};