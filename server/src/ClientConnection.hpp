#ifndef CLIENTCONNECTIONH
#define CLIENTCONNECTIONH

#include <cstdio>
#include <ctime>

#include "sox.hpp"
#include "ByteStream.hpp"

namespace ZeldaOnline
{
	class ClientConnection
	{
	public:
		explicit ClientConnection(SoxHandle socketId);
		~ClientConnection();

		ClientConnection(const ClientConnection&) = delete;
		ClientConnection& operator=(const ClientConnection&) = delete;

		SoxHandle Socket() const {
			return m_socketId;
		}
		bool Connected() const {
			return m_connected;
		}
		time_t ConnectedAt() const {
			return m_connectedAt;
		}
		time_t LastReceivedAt() const {
			return m_lastReceivedAt;
		}
		time_t SecondsSinceLastReceived() const {
			return time(nullptr) - m_lastReceivedAt;
		}

		int ReceiveData();

		bool NextPacket(ByteStream& out);

		bool SendPacket(const ByteStream& payload);

		void Send(const ByteStream& data);
		void Send(const char* data, unsigned int length);

		void FlushSendBuffer();

		bool HasPendingSend() const {
			return m_sendBuffer.BytesLeft() > 0;
		}

		unsigned int SendBacklog() const {
			return m_sendBuffer.BytesLeft();
		}

		bool IsSendBacklogged() const;

		void Disconnect();

		void SetClientGUID(uint64_t guid) {
			m_clientGUID = guid;
		}

		uint64_t ClientGUID() const {
			return m_clientGUID;
		}

	private:
		void CompactRecvBuffer();

		SoxHandle m_socketId;
		bool m_connected;
		time_t m_connectedAt;
		time_t m_lastReceivedAt;
		ByteStream m_recvBuffer;
		ByteStream m_sendBuffer;

		uint64_t m_clientGUID = 0;

		const char* m_disconnectReason = nullptr;
		int m_disconnectError = 0;
	};
}

#endif