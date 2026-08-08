#include "ZNetworking.hpp"

#include <cstdio>
#include <string>

namespace {

void EnsureSoxInitialised() {
    static bool s_done = false;
    if (!s_done) {
        soxInitialize();
        s_done = true;
    }
}

bool ConnectInProgress(int err) {
#if defined(_WIN32) || defined(WIN32)
    return err == EWOULDBLOCK;
#else
    return err == EINPROGRESS || err == EWOULDBLOCK || err == EINTR;
#endif
}

}

namespace ZeldaOnline {

ZNetworking::~ZNetworking() {
    if (m_socket != INVALID_SOCKET) {
        soxCloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    if (m_address != nullptr) {
        soxReleaseAddress(m_address);
        m_address = nullptr;
    }
}

bool ZNetworking::Enable(const char* host, uint16_t port) {
    if (isEnabled) {
        return true;
    }

    EnsureSoxInitialised();
    disconnectReason.clear();
    m_sendBuffer.Clear();

    if (!soxResolveHost(host, &m_address) || m_address == nullptr) {
        disconnectReason = "ResolveHost failed for " + std::string(host);
        std::printf("[ZNetworking] %s\n", disconnectReason.c_str());
        m_address = nullptr;
        return false;
    }

    m_socket = soxCreateTcpSocket(m_address);
    if (m_socket == INVALID_SOCKET) {
        disconnectReason = "CreateTcpSocket err=" + std::to_string(soxLastError());
        std::printf("[ZNetworking] %s\n", disconnectReason.c_str());
        soxReleaseAddress(m_address);
        m_address = nullptr;
        return false;
    }

    soxUnblockSocket(m_socket);
    soxDisableNagle(m_socket);

    isEnabled = true;
    isConnected = false;
    m_connectStartedAt = time(nullptr);

    if (soxTcpConnect(m_socket, m_address, port)) {
        isConnected = true;
        std::printf("[ZNetworking] Connected to %s:%u\n", host, static_cast<unsigned>(port));
        OnConnected();
        return true;
    }

    const int err = soxLastError();
    if (!ConnectInProgress(err)) {
        Close("Connect failed err=" + std::to_string(err));
        return false;
    }

    std::printf("[ZNetworking] Connecting to %s:%u...\n", host, static_cast<unsigned>(port));
    return true;
}

void ZNetworking::Disable() {
    if (!isEnabled) {
        return;
    }
    Close("Disable()");
}

void ZNetworking::Close(const std::string& reason) {
    const bool wasUp = isEnabled || isConnected;

    if (m_socket != INVALID_SOCKET) {
        soxCloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    if (m_address != nullptr) {
        soxReleaseAddress(m_address);
        m_address = nullptr;
    }
    m_sendBuffer.Clear();

    isConnected = false;
    isEnabled = false;

    if (!wasUp) {
        OnConnectionClosedBeforeConnect();
        return;
    }

    disconnectReason = reason;
    std::printf("[ZNetworking] Disconnected: %s\n", reason.c_str());
    OnDisconnected();
}

bool ZNetworking::CompleteConnect() {
    if (soxTcpConnected(m_socket)) {
        isConnected = true;
        std::printf("[ZNetworking] Connection established\n");
        OnConnected();
        return true;
    }

    if (m_connectTimeout > 0 && time(nullptr) - m_connectStartedAt > m_connectTimeout) {
        Close("Connect timed out after " + std::to_string(m_connectTimeout) + "s");
    }
    return false;
}

void ZNetworking::Poll() {
    if (!isEnabled || m_socket == INVALID_SOCKET) {
        return;
    }

    if (!isConnected) {
        if (!CompleteConnect()) {
            return;
        }
        if (!isConnected) {
            return;
        }
    }

    for (;;) {
        char chunk[RECV_CHUNK];
        int r = soxTcpRead(m_socket, chunk, sizeof(chunk));

        if (r > 0) {
            OnIncomingData(chunk, r);
            if (!isConnected) {
                return;
            }
            continue;
        }
        if (r == 0) {
            Close("Peer closed the connection");
            return;
        }

        const int err = soxLastError();
        if (err == EWOULDBLOCK) {
            break;
        }
#if !defined(_WIN32) && !defined(WIN32)
        if (err == EINTR) {
            continue;
        }
#endif
        Close("Recv err=" + std::to_string(err));
        return;
    }

    ProcessOutgoingPackets();
    if (isConnected) {
        FlushSendBuffer();
    }
}

bool ZNetworking::SendDataToRemote(const char* payload, int length) {
    if (!isConnected || m_socket == INVALID_SOCKET) {
        std::printf("[ZNetworking] SendDataToRemote while not connected\n");
        return false;
    }
    if (length <= 0) {
        return true;
    }

    m_sendBuffer.Write(payload, (unsigned int)(length));
    FlushSendBuffer();
    return true;
}

void ZNetworking::FlushSendBuffer() {
    if (!isConnected || m_socket == INVALID_SOCKET) {
        return;
    }

    while (m_sendBuffer.BytesLeft() > 0) {
        const char* data = m_sendBuffer.Text() + m_sendBuffer.TellRead();
        int r = soxTcpWrite(m_socket, data, m_sendBuffer.BytesLeft());

        if (r > 0) {
            m_sendBuffer.SeekRead(r, ByteStream::ORIGIN_CUR);
            continue;
        }

        const int err = soxLastError();
        if (r < 0 && err == EWOULDBLOCK) {
            break;
        }
#if !defined(_WIN32) && !defined(WIN32)
        if (r < 0 && err == EINTR) {
            continue;
        }
#endif
        Close("Send r=" + std::to_string(r) + " err=" + std::to_string(err));
        return;
    }

    m_sendBuffer.Compact();
}

}
