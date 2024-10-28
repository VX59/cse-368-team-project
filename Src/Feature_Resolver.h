#include "player_ent.h"

struct vec
{
    union
    {
        struct { float x, y, z; };
        float v[3];
        int i[3];
    };
};

struct traceresult_s
{
     vec end;
     bool collided;
};

struct Features
{
    traceresult_s rays[4];
};

class Feature_Resolver
{
public:
    Features *features;
    player_ent *player1;

    void (*TraceLine)(vec from, vec to, __uint64_t pTracer, bool CheckPlayers, traceresult_s *tr);

    void Ray_Trace(float yaw_offset, float pitch_offset);
    Feature_Resolver(player_ent *p, Features *f)
    {
        player1 = p;
        features = f;
    };
};