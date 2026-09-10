#include <cstring>

#include "ClientConnection.hpp"
#include "BytePacking.hpp"

namespace ZeldaOnline
{
	static const unsigned int RECV_COMPACT_THRESHOLD = 1024 * 1024;
	static const unsigned int SEND_BACKLOG_WARN = 512 * 1024;
	static const unsigned int SEND_BACKLOG_MAX = 4 * 1024 * 1024;

	ClientConnection::ClientConnection(SoxHandle socketId)
		: m_socketId(socketId), m_connected(true), m_connectedAt(time(nullptr)), m_lastReceivedAt(time(nullptr))
	{
		soxUnblockSocket(socketId);
		soxDisableNagle(socketId);
		soxEnableKeepAlive(socketId);
	}

	ClientConnection::~ClientConnection()
	{
		Disconnect();
	}

	int ClientConnection::ReceiveData()
	{
		if (!m_connected)
			return 0;

		char chunk[4096];
		int total = 0;

		for (;;)
		{
			int r = soxTcpRead(m_socketId, chunk, sizeof(chunk));
			if (r > 0)
			{
				m_recvBuffer.Write(chunk, (unsigned int)(r));
				m_lastReceivedAt = time(nullptr);
				total += r;
				continue;
			}
			if (r == 0)
			{
				m_disconnectReason = "peer closed";
				Disconnect();
				break;
			}
			if (soxLastError() == EWOULDBLOCK)
				break;
#if !defined(_WIN32) && !defined(WIN32)
			if (soxLastError() == EINTR)
				continue;
#endif
			m_disconnectReason = "read error";
			m_disconnectError = soxLastError();
			Disconnect();
			break;
		}
		return total;
	}

	bool ClientConnection::NextPacket(ByteStream& out)
	{
		if (m_recvBuffer.BytesLeft() < 2)
		{
			CompactRecvBuffer();
			return false;
		}

		PackedUInt2 lenField;
		memcpy(&lenField, m_recvBuffer.Text() + m_recvBuffer.TellRead(), 2);
		unsigned int payloadLen = lenField.value();

		if (m_recvBuffer.BytesLeft() < 2 + payloadLen)
		{
			CompactRecvBuffer();
			return false;
		}

		m_recvBuffer.SeekRead(2, ByteStream::ORIGIN_CUR);
		out = m_recvBuffer.Read(payloadLen);
		return true;
	}

	void ClientConnection::CompactRecvBuffer()
	{
		if (m_recvBuffer.TellRead() >= RECV_COMPACT_THRESHOLD)
			m_recvBuffer.Compact();
	}

	bool ClientConnection::SendPacket(const ByteStream& payload)
	{
		if (!m_connected)
			return false;
		if (payload.Length() > 0xFFFF)
			return false;

		if (m_sendBuffer.BytesLeft() >= SEND_BACKLOG_MAX)
		{
			m_disconnectReason = "send backlog exceeded";
			Disconnect();
			return false;
		}

		m_sendBuffer.Reserve(payload.Length() + 2);
		m_sendBuffer << PackedUInt2(payload.Length());
		m_sendBuffer.Write(payload.Text(), payload.Length());
		FlushSendBuffer();
		return true;
	}

	void ClientConnection::Send(const ByteStream& data)
	{
		if (!m_connected)
			return;
		m_sendBuffer.Write(data.Text(), data.Length());
		FlushSendBuffer();
	}

	void ClientConnection::Send(const char* data, unsigned int length)
	{
		if (!m_connected)
			return;
		m_sendBuffer.Write(data, length);
		FlushSendBuffer();
	}

	void ClientConnection::FlushSendBuffer()
	{
		if (!m_connected)
			return;

		while (m_sendBuffer.BytesLeft() > 0)
		{
			const char* data = m_sendBuffer.Text() + m_sendBuffer.TellRead();
			int r = soxTcpWrite(m_socketId, data, m_sendBuffer.BytesLeft());

			if (r > 0)
			{
				m_sendBuffer.SeekRead(r, ByteStream::ORIGIN_CUR);
				continue;
			}

			if (r == 0)
				break;

			if (soxLastError() == EWOULDBLOCK)
				break;

#if !defined(_WIN32) && !defined(WIN32)
			if (soxLastError() == EINTR)
				continue;
#endif

			m_disconnectReason = "write error";
			m_disconnectError = soxLastError();
			Disconnect();
			return;
		}

		m_sendBuffer.Compact();
	}

	bool ClientConnection::IsSendBacklogged() const
	{
		return m_sendBuffer.BytesLeft() >= SEND_BACKLOG_WARN;
	}

	void ClientConnection::Disconnect()
	{
		if (!m_connected)
			return;

		if (m_disconnectReason == nullptr)
			m_disconnectReason = "closed locally";

		std::printf("ZeldaOnline: connection closed (%s, err=%d, sendBacklog=%u)\n",
			m_disconnectReason, m_disconnectError, m_sendBuffer.BytesLeft());

		m_connected = false;
		soxCloseSocket(m_socketId);
	}
}