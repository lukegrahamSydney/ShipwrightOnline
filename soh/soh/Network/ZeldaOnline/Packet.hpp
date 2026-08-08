#ifndef PACKETH
#define PACKETH

#include "BytePacking.hpp"
#include "ByteStream.hpp"

namespace ZeldaOnline
{
	inline ByteStream newPacket(int packetId) {
		return (ByteStream() << PackedUInt1((unsigned int)(packetId)));
	}
}

#endif
