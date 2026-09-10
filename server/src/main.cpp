#include <cstring>
#include <cstdlib>
#include "OOTServer.hpp"

int main(int argc, char** argv)
{
    int port = 21050;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            port = atoi(argv[++i]);
    }
    ZeldaOnline::OOTServer server(port);
    return server.run();
}
