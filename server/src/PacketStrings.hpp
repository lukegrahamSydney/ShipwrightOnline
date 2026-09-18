#ifndef PACKETSTRINGSH
#define PACKETSTRINGSH

#include <string>

#include "ByteStream.hpp"
#include "BytePacking.hpp"

namespace ZeldaOnline {

inline std::string ReadString1(ByteStream& packet) {
    if (packet.BytesLeft() < 1)
        return "";

    unsigned int len = packet.Read<PackedUInt1>().value();

    if (packet.BytesLeft() < len) {
        packet.ReadString(packet.BytesLeft());
        return "";
    }

    return packet.ReadString(len);
}

inline std::string ReadString2(ByteStream& packet) {
    if (packet.BytesLeft() < 2)
        return "";

    unsigned int len = packet.Read<PackedUInt2>().value();

    if (packet.BytesLeft() < len) {
        packet.ReadString(packet.BytesLeft());
        return "";
    }

    return packet.ReadString(len);
}

inline std::string ReadStringVar(ByteStream& packet) {
    if (packet.BytesLeft() < 1)
        return "";

    unsigned int len = packet.ReadVarUInt();

    if (packet.BytesLeft() < len) {
        packet.ReadString(packet.BytesLeft());
        return "";
    }

    return packet.ReadString(len);
}

} // namespace ZeldaOnline

#endif