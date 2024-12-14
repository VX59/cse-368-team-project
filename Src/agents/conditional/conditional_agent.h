#pragma once
#include "../../Feature_Resolver.h"
#include "../../Environment_Interaction.h"

class ConditionalAgent
{

private:
    Features *features;
    Environment_Interaction *interface;
public:
    void run();
    ConditionalAgent(Features *features, Environment_Interaction *interface);
};