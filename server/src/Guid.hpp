#ifndef GUIDH
#define GUIDH
#include <random>

namespace ZeldaOnline
{
    inline uint64_t NewGuid()
    {
        static std::mt19937_64 rng{ std::random_device{}() };
        static std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);
        return dist(rng);
    }
}

#endif

