#ifndef ABSTRACTACTORH
#define ABSTRACTACTORH

#include "BytePacking.hpp"
#include "ByteStream.hpp"
#include "Guid.hpp"

namespace ZeldaOnline
{
	enum class NetActorType
	{
		PLAYER,
		WORLD_ACTOR,
	};

	class AbstractActor
	{
	public:
		explicit AbstractActor(int networkID, int actorID) : m_networkID(networkID), m_actorID(actorID), m_guid(NewGuid()) {}
		virtual ~AbstractActor() {}

		virtual NetActorType Type() const = 0;

		int NetworkID() const {
			return m_networkID;
		}

		void SetPosition(float x, float y, float z) {
			m_x = x; m_y = y; m_z = z;
		}
		void SetRotation(short rotX, short rotY, short rotZ) {
			m_rotX = rotX; m_rotY = rotY; m_rotZ = rotZ;
		}
		float X() const { return m_x; }
		float Y() const { return m_y; }
		float Z() const { return m_z; }
		short RotX() const { return m_rotX; }
		short RotY() const { return m_rotY; }
		short RotZ() const { return m_rotZ; }

		int RoomIndex() const { return m_roomIndex; }
		void SetRoomIndex(int index) { m_roomIndex = index; }
		bool IsSceneScoped() const { return m_roomIndex < 0; }
		int ActorID() const {
			return m_actorID;
		}

		bool HasProperties() const {
			return !m_properties.empty();
		}

		ByteStream FullProperties() const {
			ByteStream out;
			for (const auto& kv : m_properties) {
				out.WriteVarUInt(kv.first);
				out.WriteVarUInt(kv.second.Length());
				out.Write(kv.second);
			}
			return out;
		}

		ByteStream FullPropertiesAndStandard() const {
			ByteStream out;

			auto emit = [&out](unsigned int id, const ByteStream& value) {
				out.WriteVarUInt(id);
				out.WriteVarUInt(value.Length());
				out.Write(value);
				};

			emit(0, ByteStream() << PackedFloat4(X()));
			emit(1, ByteStream() << PackedFloat4(Y()));
			emit(2, ByteStream() << PackedFloat4(Z()));
			emit(3, ByteStream() << PackedInt2((int)(RotX())));
			emit(4, ByteStream() << PackedInt2((int)(RotY())));
			emit(5, ByteStream() << PackedInt2((int)(RotZ())));
			emit(6, ByteStream() << PackedInt1(RoomIndex()));

			for (const auto& kv : m_properties) {
				emit(unsigned(kv.first), kv.second);
			}
			return out;
		}

		void MergeCustomState(ByteStream blob) {
			blob.RewindRead();

			bool havePos = false, haveRot = false;
			float px = X(), py = Y(), pz = Z();
			short rx = RotX(), ry = RotY(), rz = RotZ();

			while (blob.BytesLeft() >= 1) {
				unsigned int index = blob.ReadVarUInt();
				unsigned int len = blob.ReadVarUInt();
				if (blob.BytesLeft() < len) break;

				ByteStream value = blob.Read(len);

				switch (index) {
				case 0: px = value.Read<PackedFloat4>().value(); havePos = true; break;
				case 1: py = value.Read<PackedFloat4>().value(); havePos = true; break;
				case 2: pz = value.Read<PackedFloat4>().value(); havePos = true; break;
				case 3: rx = (short)(value.Read<PackedInt2>().value()); haveRot = true; break;
				case 4: ry = (short)(value.Read<PackedInt2>().value()); haveRot = true; break;
				case 5: rz = (short)(value.Read<PackedInt2>().value()); haveRot = true; break;
				case 6: {
					m_roomIndex = (int)(value.Read<PackedInt1>().value());
					break;
				}
				default: m_properties[index] = value; break;
				}
			}

			if (havePos) SetPosition(px, py, pz);
			if (haveRot) SetRotation(rx, ry, rz);
		}

		uint64_t Guid() const { return m_guid; }

	private:
		int m_networkID;
		int m_actorID = 0;
		float m_x = 0.0f, m_y = 0.0f, m_z = 0.0f;
		short m_rotX = 0, m_rotY = 0, m_rotZ = 0;
		int m_roomIndex = 0;
		std::unordered_map<int, ByteStream> m_properties;

		uint64_t m_guid;

	};
}

#endif
