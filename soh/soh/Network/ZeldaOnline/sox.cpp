#include <cstring>
#include "sox.hpp"

void soxInitialize()
{
#if defined(_WIN32) || defined(WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

void soxCleanup()
{
#if defined(_WIN32) || defined(WIN32)
    WSACleanup();
#endif
}

int soxLastError()
{
#if defined(_WIN32) || defined(WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

SoxHandle soxCreateTcpSocket()
{
    SoxHandle retval = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    int yes = 1;

    if (retval != INVALID_SOCKET)
    {
        int no = 0;
        setsockopt(retval, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
    }
    else
    {
        retval = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (retval == INVALID_SOCKET)
            return retval;
    }

    setsockopt(retval, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    return retval;
}

SoxHandle soxCreateTcpSocket(struct addrinfo* res)
{
    SoxHandle retval = socket(res->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (retval == INVALID_SOCKET)
        return retval;

    if (res->ai_family == AF_INET6)
    {
        int no = 0;
        setsockopt(retval, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
    }
    return retval;
}

SoxHandle soxCreateUdpSocket(int family)
{
    SoxHandle retval = socket(family, SOCK_DGRAM, 0);
    if (retval == INVALID_SOCKET)
        return retval;

    if (family == AF_INET6)
    {
        int no = 0;
        setsockopt(retval, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
    }
    return retval;
}

bool soxResolveHost(const char* hostName, struct addrinfo** res)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    return getaddrinfo(hostName, NULL, &hints, res) == 0;
}

void soxReleaseAddress(struct addrinfo* res)
{
    freeaddrinfo(res);
}

const char* soxIpString(const struct sockaddr* sa, char* out, size_t outLen)
{
    if (!sa || !out || outLen == 0)
        return NULL;

    if (sa->sa_family == AF_INET)
    {
        const struct sockaddr_in* v4 = (const struct sockaddr_in*)sa;
        return inet_ntop(AF_INET, &v4->sin_addr, out, (socklen_t)outLen);
    }
    if (sa->sa_family == AF_INET6)
    {
        const struct sockaddr_in6* v6 = (const struct sockaddr_in6*)sa;
        if (IN6_IS_ADDR_V4MAPPED(&v6->sin6_addr))
        {
            struct in_addr v4addr;
            memcpy(&v4addr, ((const unsigned char*)&v6->sin6_addr) + 12, 4);
            return inet_ntop(AF_INET, &v4addr, out, (socklen_t)outLen);
        }
        return inet_ntop(AF_INET6, &v6->sin6_addr, out, (socklen_t)outLen);
    }
    return NULL;
}

bool soxTcpIp(SoxHandle socketId, struct sockaddr_in6* out)
{
    memset(out, 0, sizeof(*out));
    socklen_t addrSize = sizeof(*out);
    return getpeername(socketId, (struct sockaddr*)out, &addrSize) == 0;
}

void soxCloseSocket(SoxHandle socketId)
{
#if defined(_WIN32) || defined(WIN32)
    closesocket(socketId);
#else
    close(socketId);
#endif
}

void soxShutdownSocket(SoxHandle socketId)
{
#if defined(_WIN32) || defined(WIN32)
    shutdown(socketId, SD_SEND);
#else
    shutdown(socketId, SHUT_WR);
#endif
}

void soxBlockSocket(SoxHandle socketId)
{
#if defined(_WIN32) || defined(WIN32)
    unsigned long opt = 0;
    ioctlsocket(socketId, FIONBIO, &opt);
#else
    int flags = fcntl(socketId, F_GETFL);
    if (flags != -1)
        fcntl(socketId, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

void soxUnblockSocket(SoxHandle socketId)
{
#if defined(_WIN32) || defined(WIN32)
    unsigned long opt = 1;
    ioctlsocket(socketId, FIONBIO, &opt);
#else
    int flags = fcntl(socketId, F_GETFL);
    if (flags != -1)
        fcntl(socketId, F_SETFL, flags | O_NONBLOCK);
#endif
}

bool soxTcpConnect(SoxHandle socketId, struct addrinfo* address, int port)
{
    if (address->ai_family == AF_INET6)
    {
        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)address->ai_addr;
        ipv6->sin6_family = AF_INET6;
        ipv6->sin6_port = htons((unsigned short)port);
        return connect(socketId, (const struct sockaddr*)ipv6, sizeof(struct sockaddr_in6)) != -1;
    }
    if (address->ai_family == AF_INET)
    {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)address->ai_addr;
        ipv4->sin_family = AF_INET;
        ipv4->sin_port = htons((unsigned short)port);
        return connect(socketId, (const struct sockaddr*)ipv4, sizeof(struct sockaddr_in)) != -1;
    }
    return false;
}

int soxTcpWrite(SoxHandle socketId, const char* data, size_t len)
{
    return (int)send(socketId, data, (int)len, 0);
}

int soxTcpWriteString(SoxHandle socketId, const char* str)
{
    return (int)send(socketId, str, (int)strlen(str), 0);
}

int soxTcpRead(SoxHandle socketId, char* out, size_t outLen)
{
    return (int)recv(socketId, out, (int)outLen, 0);
}

int soxTcpReadLine(SoxHandle socketId, char* out, size_t outLen)
{
    int size = (int)recv(socketId, out, (int)(outLen - 1), MSG_PEEK);

    if (size > 0)
    {
        int recvCount = 0;
        while (recvCount < size)
        {
            if (out[recvCount++] == '\n')
                break;
        }

        size = (int)recv(socketId, out, recvCount, 0);
        if (size > 0)
            out[size] = 0;
    }
    return size;
}

int soxTcpPeek(SoxHandle socketId, char* out, size_t outLen)
{
    return (int)recv(socketId, out, (int)outLen, MSG_PEEK);
}

bool soxTcpConnected(SoxHandle socketId)
{
    fd_set writeSet, errSet;
    struct timeval timeout;
    FD_ZERO(&writeSet);
    FD_ZERO(&errSet);
    memset(&timeout, 0, sizeof(timeout));
    FD_SET(socketId, &writeSet);
    FD_SET(socketId, &errSet);

    int r = select((int)socketId + 1, NULL, &writeSet, &errSet, &timeout);
    if (r <= 0)
        return false;
    if (FD_ISSET(socketId, &errSet))
        return false;

    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(socketId, SOL_SOCKET, SO_ERROR, (char*)&err, &len) != 0)
        return false;
    return err == 0;
}

void soxEnableNagle(SoxHandle socketId)
{
    int opt = 0;
    setsockopt(socketId, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
}

void soxDisableNagle(SoxHandle socketId)
{
    int opt = 1;
    setsockopt(socketId, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
}

bool soxBindUdpPort(SoxHandle socketId, int port, int family)
{
    if (family == AF_INET6)
    {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = in6addr_any;
        addr.sin6_port = htons((unsigned short)port);
        return bind(socketId, (struct sockaddr*)&addr, sizeof(addr)) != -1;
    }
    else
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons((unsigned short)port);
        return bind(socketId, (struct sockaddr*)&addr, sizeof(addr)) != -1;
    }
}

int soxUdpWrite(SoxHandle socketId, const char* data, size_t len, struct sockaddr_in6* addrIP, int port)
{
    if (addrIP->sin6_family == AF_INET6)
    {
        struct sockaddr_in6 addr;
        memcpy(&addr, addrIP, sizeof(addr));
        addr.sin6_port = htons((unsigned short)port);
        return (int)sendto(socketId, data, (int)len, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in6));
    }
    else
    {
        struct sockaddr_in addr;
        memcpy(&addr, addrIP, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((unsigned short)port);
        return (int)sendto(socketId, data, (int)len, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    }
}

int soxUdpRead(SoxHandle socketId, char* out, size_t outLen, struct sockaddr_in6* addrIP)
{
    if (addrIP != NULL)
    {
        memset(addrIP, 0, sizeof(*addrIP));
        socklen_t addrSize = sizeof(*addrIP);
        return (int)recvfrom(socketId, out, (int)outLen, 0, (struct sockaddr*)addrIP, &addrSize);
    }
    return (int)recvfrom(socketId, out, (int)outLen, 0, NULL, NULL);
}

int soxUdpPeek(SoxHandle socketId, char* out, size_t outLen)
{
    struct sockaddr_in6 addr;
    socklen_t addrSize = sizeof(addr);
    return (int)recvfrom(socketId, out, (int)outLen, MSG_PEEK, (struct sockaddr*)&addr, &addrSize);
}

bool soxTcpListen(SoxHandle socketId, int port)
{
    int no = 0;
    setsockopt(socketId, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_addr = in6addr_any;
    addr6.sin6_port = htons((unsigned short)port);

    if (bind(socketId, (struct sockaddr*)&addr6, sizeof(addr6)) == -1)
    {
        struct sockaddr_in addr4;
        memset(&addr4, 0, sizeof(addr4));
        addr4.sin_family = AF_INET;
        addr4.sin_addr.s_addr = htonl(INADDR_ANY);
        addr4.sin_port = htons((unsigned short)port);

        if (bind(socketId, (struct sockaddr*)&addr4, sizeof(addr4)) == -1)
            return false;
    }

    return listen(socketId, 50) != -1;
}

SoxHandle soxTcpAccept(SoxHandle socketId)
{
    return accept(socketId, NULL, NULL);
}

void soxUnlockPort(SoxHandle socketId)
{
    int on = 1;
    setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
}

#ifdef __cplusplus

namespace sox
{
    SoxPoller::SoxPoller()
    {
        Clear();
    }

    void SoxPoller::Clear()
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_ZERO(&errorSet);
        range = 0;
    }

    void SoxPoller::AddSocket(SoxHandle soxHandle, int options)
    {
        if (options & POLL_READ)
            FD_SET(soxHandle, &readSet);

        if (options & POLL_WRITE)
            FD_SET(soxHandle, &writeSet);

        if (options & POLL_ERROR)
            FD_SET(soxHandle, &errorSet);

        if (soxHandle > range)
            range = soxHandle;
    }

    bool SoxPoller::IsSet(SoxHandle soxHandle, int option) const
    {
        if (option == POLL_READ)
            return FD_ISSET(soxHandle, &readSet) != 0;

        if (option == POLL_WRITE)
            return FD_ISSET(soxHandle, &writeSet) != 0;

        if (option == POLL_ERROR)
            return FD_ISSET(soxHandle, &errorSet) != 0;

        return false;
    }

    int SoxPoller::Poll(int timeOutMicros)
    {
        struct timeval timev;
        timev.tv_sec = timeOutMicros / 1000000;
        timev.tv_usec = timeOutMicros % 1000000;

        return select((int)range + 1, &readSet, &writeSet, &errorSet,
            (timeOutMicros >= 0 ? &timev : NULL));
    }
}

#endif
