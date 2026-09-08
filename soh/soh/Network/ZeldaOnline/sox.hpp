#ifndef SOXH
#define SOXH

#define POLL_WRITE 1
#define POLL_READ 2
#define POLL_ERROR 4

#if defined(_WIN32) || defined(WIN32)
#include <winsock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK

typedef SOCKET SoxHandle;
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
typedef int SoxHandle;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

#include <stddef.h>

void soxInitialize();
void soxCleanup();
int soxLastError();

SoxHandle soxCreateTcpSocket();
SoxHandle soxCreateTcpSocket(struct addrinfo* res);
SoxHandle soxCreateUdpSocket(int family);

bool soxResolveHost(const char* hostName, struct addrinfo** res);
void soxReleaseAddress(struct addrinfo* res);

const char* soxIpString(const struct sockaddr* sa, char* out, size_t outLen);

bool soxTcpIp(SoxHandle socketId, struct sockaddr_in6* out);

void soxCloseSocket(SoxHandle socketId);
void soxShutdownSocket(SoxHandle socketId);
void soxBlockSocket(SoxHandle socketId);
void soxUnblockSocket(SoxHandle socketId);

bool soxTcpConnect(SoxHandle socketId, struct addrinfo* addr, int port);
int soxTcpWrite(SoxHandle socketId, const char* data, size_t len);
int soxTcpWriteString(SoxHandle socketId, const char* str);
int soxTcpRead(SoxHandle socketId, char* out, size_t outLen);
int soxTcpReadLine(SoxHandle socketId, char* out, size_t outLen);
int soxTcpPeek(SoxHandle socketId, char* out, size_t outLen);

bool soxTcpConnected(SoxHandle socketId);

void soxEnableNagle(SoxHandle socketId);
void soxDisableNagle(SoxHandle socketId);
void soxEnableKeepAlive(SoxHandle socketId);

bool soxBindUdpPort(SoxHandle socketId, int port, int family);
int soxUdpWrite(SoxHandle socketId, const char* data, size_t len, struct sockaddr_in6* addrIP, int port);
int soxUdpRead(SoxHandle socketId, char* out, size_t outLen, struct sockaddr_in6* addrIP);
int soxUdpPeek(SoxHandle socketId, char* out, size_t outLen);

bool soxTcpListen(SoxHandle socketId, int port);
SoxHandle soxTcpAccept(SoxHandle socketId);
void soxUnlockPort(SoxHandle socketId);

#ifdef __cplusplus

namespace sox
{
    class SoxPoller
    {
    private:
        fd_set readSet,
            writeSet,
            errorSet;
        SoxHandle range;

    public:
        SoxPoller();
        void Clear();
        void AddSocket(SoxHandle soxHandle, int options);
        bool IsSet(SoxHandle soxHandle, int option) const;

        int Poll(int timeOutMicros);
    };
}

#endif
#endif
