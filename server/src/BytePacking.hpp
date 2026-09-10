#ifndef BYTEPACKINGH
#define BYTEPACKINGH

#include <cstdint>
#include <cstring>

namespace ZeldaOnline
{

	struct PackedInt1
	{
		PackedInt1() {}
		explicit PackedInt1(int value) {
			b = static_cast<std::uint8_t>(value & 0xFF);
		}
		int value() const {
			return static_cast<std::int8_t>(b);
		}
		std::uint8_t b = 0;
	};

	struct PackedUInt1
	{
		PackedUInt1() {}
		explicit PackedUInt1(unsigned int value) {
			b = static_cast<std::uint8_t>(value & 0xFF);
		}
		unsigned int value() const {
			return b;
		}
		std::uint8_t b = 0;
	};

	struct PackedInt2
	{
		PackedInt2() {}
		explicit PackedInt2(int value) {
			b[0] = static_cast<std::uint8_t>(value & 0xFF);
			b[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
		}
		int value() const {
			std::uint32_t u = (std::uint32_t(b[1]) << 8) | b[0];
			if (u & 0x8000u) u |= 0xFFFF0000u;
			return static_cast<std::int32_t>(u);
		}
		std::uint8_t b[2] = {};
	};

	struct PackedUInt2
	{
		PackedUInt2() {}
		explicit PackedUInt2(unsigned int value) {
			b[0] = static_cast<std::uint8_t>(value & 0xFF);
			b[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
		}
		unsigned int value() const {
			return (std::uint32_t(b[1]) << 8) | b[0];
		}
		std::uint8_t b[2] = {};
	};

	struct PackedInt3
	{
		PackedInt3() {}
		explicit PackedInt3(int value) {
			b[0] = static_cast<std::uint8_t>(value & 0xFF);
			b[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
			b[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
		}
		int value() const {
			std::uint32_t u = (std::uint32_t(b[2]) << 16)
				| (std::uint32_t(b[1]) << 8)
				| b[0];
			if (u & 0x800000u) u |= 0xFF000000u;
			return static_cast<std::int32_t>(u);
		}
		std::uint8_t b[3] = {};
	};

	struct PackedUInt3
	{
		PackedUInt3() {}
		explicit PackedUInt3(unsigned int value) {
			b[0] = static_cast<std::uint8_t>(value & 0xFF);
			b[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
			b[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
		}
		unsigned int value() const {
			return (std::uint32_t(b[2]) << 16)
				| (std::uint32_t(b[1]) << 8)
				| b[0];
		}
		std::uint8_t b[3] = {};
	};

	struct PackedInt4
	{
		PackedInt4() {}
		explicit PackedInt4(int value) {
			std::uint32_t u = static_cast<std::uint32_t>(value);
			b[0] = static_cast<std::uint8_t>(u & 0xFF);
			b[1] = static_cast<std::uint8_t>((u >> 8) & 0xFF);
			b[2] = static_cast<std::uint8_t>((u >> 16) & 0xFF);
			b[3] = static_cast<std::uint8_t>((u >> 24) & 0xFF);
		}
		int value() const {
			std::uint32_t u = (std::uint32_t(b[3]) << 24)
				| (std::uint32_t(b[2]) << 16)
				| (std::uint32_t(b[1]) << 8)
				| b[0];
			return static_cast<std::int32_t>(u);
		}
		std::uint8_t b[4] = {};
	};

	struct PackedUInt4
	{
		PackedUInt4() {}
		explicit PackedUInt4(unsigned int value) {
			b[0] = static_cast<std::uint8_t>(value & 0xFF);
			b[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
			b[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
			b[3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
		}
		unsigned int value() const {
			return (std::uint32_t(b[3]) << 24)
				| (std::uint32_t(b[2]) << 16)
				| (std::uint32_t(b[1]) << 8)
				| b[0];
		}
		std::uint8_t b[4] = {};
	};

	struct PackedInt8
	{
		PackedInt8() {}
		explicit PackedInt8(std::int64_t value) {
			std::uint64_t u = static_cast<std::uint64_t>(value);
			for (int i = 0; i < 8; ++i)
				b[i] = static_cast<std::uint8_t>((u >> (8 * i)) & 0xFF);
		}
		std::int64_t value() const {
			std::uint64_t u = 0;
			for (int i = 7; i >= 0; --i)
				u = (u << 8) | b[i];
			return static_cast<std::int64_t>(u);
		}
		std::uint8_t b[8] = {};
	};

	struct PackedUInt8
	{
		PackedUInt8() {}
		explicit PackedUInt8(std::uint64_t value) {
			for (int i = 0; i < 8; ++i)
				b[i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xFF);
		}
		std::uint64_t value() const {
			std::uint64_t u = 0;
			for (int i = 7; i >= 0; --i)
				u = (u << 8) | b[i];
			return u;
		}
		std::uint8_t b[8] = {};
	};

	struct PackedFloat4
	{
		PackedFloat4() {}
		explicit PackedFloat4(float value) {
			std::uint32_t u;
			std::memcpy(&u, &value, 4);
			b[0] = static_cast<std::uint8_t>(u & 0xFF);
			b[1] = static_cast<std::uint8_t>((u >> 8) & 0xFF);
			b[2] = static_cast<std::uint8_t>((u >> 16) & 0xFF);
			b[3] = static_cast<std::uint8_t>((u >> 24) & 0xFF);
		}
		float value() const {
			std::uint32_t u = (std::uint32_t(b[3]) << 24)
				| (std::uint32_t(b[2]) << 16)
				| (std::uint32_t(b[1]) << 8)
				| b[0];
			float f;
			std::memcpy(&f, &u, 4);
			return f;
		}
		std::uint8_t b[4] = {};
	};

}

#endif
