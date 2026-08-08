#ifndef ZNETWORKING_H
#define ZNETWORKING_H
#ifdef __cplusplus

#include <string>
#include <ctime>

#include "sox.hpp"
#include "ByteStream.hpp"

namespace ZeldaOnline {

class ZNetworking {
  public:
    virtual ~ZNetworking();

    bool isEnabled = false;
    bool isConnected = false;

    bool IsConnecting() const {
        return isEnabled && !isConnected;
    }

    std::string disconnectReason;

    bool Enable(const char* host, uint16_t port);

    void Disable();

    void Poll();

    bool SendDataToRemote(const char* payload, int length);

    unsigned int PendingSendBytes() const {
        return m_sendBuffer.BytesLeft();
    }

    void SetConnectTimeoutSeconds(int seconds) {
        m_connectTimeout = seconds;
    }

  protected:
    virtual void OnIncomingData(const char* payload, int length) {
    }

    virtual void OnConnected() {
    }

    virtual void OnDisconnected() {
    }

    virtual void OnConnectionClosedBeforeConnect() {
    }
    virtual void ProcessOutgoingPackets() {
    }

  private:
    void Close(const std::string& reason);
    void FlushSendBuffer();
    bool CompleteConnect();

    SoxHandle m_socket = INVALID_SOCKET;
    struct addrinfo* m_address = nullptr;

    ByteStream m_sendBuffer;
    time_t m_connectStartedAt = 0;
    int m_connectTimeout = 10;

    static const int RECV_CHUNK = 8192;
};

}

#endif
#endif
