#ifndef ABSTRACTACTORCONTROLLER_H
#define ABSTRACTACTORCONTROLLER_H
#ifdef __cplusplus


#include "ByteStream.hpp"
#include "BytePacking.hpp"
#include "PacketTypes.hpp"

#include <cstddef>
#include "functions.h"
#include <macros.h>
#include <z64.h>

extern "C" {

extern PlayState* gPlayState;
void SkelAnime_SetUpdate(SkelAnime* skelAnime);
}

namespace ZeldaOnline {

class AbstractActorController {
  private:
    bool m_startedAsLeader = false;

  public:
    static inline AbstractActorController* s_actorSoundContext = nullptr;

    enum {
        PROP_POS_X = 0,
        PROP_POS_Y,
        PROP_POS_Z,
        PROP_ROT_X,
        PROP_ROT_Y,
        PROP_ROT_Z,
        PROP_ROOM,
        PROP_SHAPE_ROT,
        PROP_EXTENDED_START,
    };

    enum {
        PROP_SHAPE_YOFFSET = PROP_EXTENDED_START,
        PROP_SCALE,
        PROP_WORLD_ROT,
        PROP_VELOCITY,
        PROP_FLAGS,
        PROP_GRAVITY,
        PROP_COLOR_FILTER,
        PROP_CUSTOM_START,
    };

    enum ColliderRole { COLL_AT = 1 << 0, COLL_AC = 1 << 1, COLL_OC = 1 << 2 };

    AbstractActorController(Actor* actor, int networkID, int sceneKey, int roomIndex, bool isLeader);

    virtual ~AbstractActorController() {
        if (m_actor != nullptr && m_actor->zoController == this) {
            if (m_actor->init == &AbstractActorController::DispatchInit) {
                m_actor->init = m_originalInit;
            }
            if (m_actor->update == &AbstractActorController::DispatchUpdate) {
                m_actor->update = m_originalUpdate;
            }
            if (m_actor->destroy == &AbstractActorController::DispatchDestroy) {
                m_actor->destroy = m_originalDestroy;
            }
            m_actor->zoController = nullptr;
        }
    }

    AbstractActorController(const AbstractActorController&) = delete;
    AbstractActorController& operator=(const AbstractActorController&) = delete;

    static void DispatchUpdate(Actor* actor, PlayState* play) {
        static_cast<AbstractActorController*>(actor->zoController)->Update(play);
    }

    static void DispatchInit(Actor* actor, PlayState* play) {
        static_cast<AbstractActorController*>(actor->zoController)->ActorInit(play);
    }

    static void InstallCustomInit(Actor* actor, ActorFunc customInit) {
        if (actor == nullptr || customInit == nullptr) {
            return;
        }

        if (actor->init == &AbstractActorController::DispatchInit && actor->zoController != nullptr) {
            static_cast<AbstractActorController*>(actor->zoController)->m_originalInit = customInit;
        } else if (actor->init != nullptr) {
            actor->init = customInit;
        }
    }

    static void DispatchDestroy(Actor* actor, PlayState* play);

    bool ShouldDelayInit() const {
        return m_networkID == 0 && m_actor && m_actor->update != nullptr;
    }

    virtual void ActorInit(PlayState* play);
    void Update(PlayState* play);

    void SendUpdate();

    static constexpr u8 CLAIM_REASON_COOLDOWN = 0;
    static constexpr u8 CLAIM_REASON_NOW = 1;

    bool ClaimLeadership(u8 reason);

    virtual bool CanRelinquishLeadership() const {
        return !IsMyOpenTextboxActor();
    }

    virtual bool ShouldLockActor() const {
        return !CanRelinquishLeadership();
    }

    void SetSpawnPosRot(const PosRot& posRot) {
        m_spawnPosRot = posRot;
    }

    void SendTriggerToPuppets(const std::string& name, const ByteStream& data);
    void SendTriggerToLeader(const std::string& name, const ByteStream& data);
    virtual void OnTrigger(const std::string& name, ByteStream& data) {
    }

    virtual bool IsPlayer() const {
        return false;
    }

    virtual bool CanSpawnActorOverNetwork(s16 actorId, s16 params) {
        return false;
    }
    void SetLeader(bool leader) {
        if (m_runningLocally)
            return;

        m_locked = false;
        if (leader == m_isLeader) {
            return;
        }
        m_isLeader = leader;
        if (leader) {
            m_forcePropResend = true;
            OnBecomeLeader();
        } else {
            OnLoseLeadership();
        }
    }

    bool IsLeader() const {
        return m_isLeader;
    }

    bool IsCreator() const {
        return m_creator;
    }
    void SetCreator(bool v) {
        m_creator = v;
    }

    int NetworkID() const {
        return m_networkID;
    }

    void SetNetworkID(int id) {
        m_networkID = id;
    }

    int SceneKey() const {
        return m_sceneKey;
    }
    int RoomIndex() const {
        return m_roomIndex;
    }
    Actor* GetActor() const {
        return m_actor;
    }

    bool IsLocked() const {
        return m_locked;
    }

    void SetLocked(bool locked) {
        m_locked = locked;
    }

    void UpdateLeadershipLock();

    void SendLeadershipLock(bool locked);

    void AppendPendingProperties(ByteStream props) {
        m_pendingProperties << props;
    }

    static AbstractActorController* CurrentLeaderContext() {
        return s_currentLeaderContext;
    }

    bool IsMyOpenTextboxActor() const {
        auto localPlayer = GET_PLAYER(gPlayState);
        return (localPlayer->stateFlags1 & PLAYER_STATE1_TALKING) && localPlayer->talkActor == m_actor;
    }

    virtual void OnServerDestroy() {
    }

    void GoLocal();

    bool IsRunningLocally() const {
        return m_runningLocally;
    }

    bool ShouldTransmitSounds() const {
        if (m_runningLocally)
            return false;
        return m_isLeader;
    }

    bool ShouldSuppressSounds() const {
        if (m_runningLocally)
            return false;
        return !m_isLeader;
    }

    void ReadProperties(ByteStream& packet) {
        if (m_runningLocally)
            return;

        s_actorSoundContext = this;
        u64 changed = 0;

        while (packet.BytesLeft() > 0) {
            unsigned int index = packet.ReadVarUInt();
            unsigned int len = packet.ReadVarUInt();
            auto pos = packet.TellRead();
            if (packet.BytesLeft() < len)
                break;

            if (!ApplyStandardProperty(index, packet, len)) {
                if (!ApplyCustomProperty(index, packet, len)) {
                    packet.SeekRead(pos + len, ByteStream::ORIGIN_SET);
                    continue;
                }
            }
            if (index < 64) {
                changed |= (u64)1 << index;
            }
            packet.SeekRead(pos + len, ByteStream::ORIGIN_SET);
        }

        OnPropertiesApplied(changed);
        s_actorSoundContext = nullptr;
        m_lastBuiltProperties.Clear();
    }

    void ApplyPendingProperties() {
        if (m_pendingProperties.Length()) {
            ReadProperties(m_pendingProperties);
            m_pendingProperties.Clear();
        }
    }

    bool UpdateAnimation(SkelAnime* skelAnime, bool lockFrame);

    virtual void OnActorInit() {
    }

    void RegisterCylinder(PlayState* play, ColliderCylinder* c, u8 roleBits);

    void RegisterColliderBase(PlayState* play, Collider* c, u8 roleBits);

    static ByteStream DiffProperties(ByteStream& prev, ByteStream& now, bool forceAll, unsigned int* count) {
        prev.RewindRead();
        now.RewindRead();

        ByteStream entries;
        *count = 0;
        bool prevExhausted = (prev.BytesLeft() == 0);

        while (now.BytesLeft() >= 1) {
            unsigned int index = now.ReadVarUInt();
            unsigned int len = now.ReadVarUInt();
            unsigned int nowDataPos = now.TellRead();
            if (now.BytesLeft() < len)
                break;

            bool changed = forceAll || prevExhausted;
            if (!changed) {
                unsigned int pindex = prev.ReadVarUInt();
                unsigned int plen = prev.ReadVarUInt();
                unsigned int prevDataPos = prev.TellRead();
                changed = (pindex != index) || (plen != len) || (now.CompareRange(nowDataPos, prev, prevDataPos, len) != 0);
                prev.Skip(plen);
            }

            if (changed) {
                entries.WriteVarUInt(index);
                entries.WriteVarUInt(len);
                entries.Write(now.Text() + nowDataPos, len);
                (*count)++;
            }
            now.Skip(len);
        }

        return entries;
    }

    virtual bool CanAcceptLeadership() const {
        return !m_runningLocally && m_actor != nullptr;
    }

    Actor* Detach() {
        Actor* actor = m_actor;
        if (actor == nullptr) {
            return nullptr;
        }

        if (actor->init != nullptr) {
            actor->init = m_originalInit;
        }
        if (actor->update != nullptr)
            actor->update = m_originalUpdate;
        actor->destroy = m_originalDestroy;
        actor->zoController = nullptr;

        m_actor = nullptr;
        return actor;
    }

  protected:
    static u8 PackColliderFlags(const Collider& c) {
        return (c.acFlags & AC_ON ? 1 : 0) | (c.atFlags & AT_ON ? 2 : 0) | (c.ocFlags1 & OC1_ON ? 4 : 0);
    }

    static void ApplyColliderFlags(Collider& c, u8 bits) {
        c.acFlags = (c.acFlags & ~AC_ON) | (bits & 1 ? AC_ON : 0);
        c.atFlags = (c.atFlags & ~AT_ON) | (bits & 2 ? AT_ON : 0);
        c.ocFlags1 = (c.ocFlags1 & ~OC1_ON) | (bits & 4 ? OC1_ON : 0);
    }

    virtual void UpdateLeader(PlayState* play) {
        m_originalUpdate(m_actor, play);
    }

    bool IsLocalPlayerClosest() const;

    void BuildStandardProperties(ByteStream& out) {
        PackProperty(PROP_POS_X, PackedFloat4(m_actor->world.pos.x), out);
        PackProperty(PROP_POS_Y, PackedFloat4(m_actor->world.pos.y), out);
        PackProperty(PROP_POS_Z, PackedFloat4(m_actor->world.pos.z), out);
        PackProperty(PROP_ROT_X, PackedInt2(m_actor->world.rot.x), out);
        PackProperty(PROP_ROT_Y, PackedInt2(m_actor->world.rot.y), out);
        PackProperty(PROP_ROT_Z, PackedInt2(m_actor->world.rot.z), out);
        PackProperty(PROP_ROOM, PackedInt1(m_actor->room), out);
        PackProperty(PROP_SHAPE_ROT,
                     ByteStream() << PackedInt2(m_actor->shape.rot.x) << PackedInt2(m_actor->shape.rot.y)
                                  << PackedInt2(m_actor->shape.rot.z),
                     out);
    }

    bool BuildStandardExtendedProperty(unsigned int which, ByteStream& out) {
        switch (which) {

            case PROP_SHAPE_YOFFSET:
                PackProperty(PROP_SHAPE_YOFFSET, PackedFloat4(m_actor->shape.yOffset), out);
                return true;
            case PROP_SCALE:
                PackProperty(PROP_SCALE,
                             ByteStream() << PackedFloat4(m_actor->scale.x) << PackedFloat4(m_actor->scale.y)
                                          << PackedFloat4(m_actor->scale.z),
                             out);
                return true;
            case PROP_VELOCITY:
                PackProperty(PROP_VELOCITY,
                             ByteStream() << PackedFloat4(m_actor->velocity.x) << PackedFloat4(m_actor->velocity.y)
                                          << PackedFloat4(m_actor->velocity.z) << PackedFloat4(m_actor->speedXZ),
                             out);
                return true;
            case PROP_FLAGS:
                PackProperty(PROP_FLAGS, PackedUInt4(m_actor->flags), out);
                return true;
            case PROP_GRAVITY:
                PackProperty(PROP_GRAVITY, PackedFloat4(m_actor->gravity), out);
                return true;

            case PROP_COLOR_FILTER:
                PackProperty(PROP_COLOR_FILTER,
                             ByteStream()
                                 << PackedUInt2(m_actor->colorFilterParams) << PackedUInt1(m_actor->colorFilterTimer),
                             out);
                return true;
            default:
                return false;
        }
    }

    virtual bool ApplyStandardProperty(unsigned int index, ByteStream& data, unsigned int propLen) {
        switch (index) {
            case PROP_POS_X:
                m_actor->world.pos.x = data.Read<PackedFloat4>().value();
                return true;
            case PROP_POS_Y:
                m_actor->world.pos.y = data.Read<PackedFloat4>().value();
                return true;
            case PROP_POS_Z:
                m_actor->world.pos.z = data.Read<PackedFloat4>().value();

                return true;
            case PROP_ROT_X:
                m_actor->world.rot.x = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_ROT_Y:
                m_actor->world.rot.y = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_ROT_Z:
                m_actor->world.rot.z = (s16)(data.Read<PackedInt2>().value());
                return true;

            case PROP_ROOM:
                m_actor->room = (s8)(data.Read<PackedInt1>().value());
                return true;

            case PROP_SHAPE_ROT:
                m_actor->shape.rot.x = (s16)data.Read<PackedInt2>().value();
                m_actor->shape.rot.y = (s16)data.Read<PackedInt2>().value();
                m_actor->shape.rot.z = (s16)data.Read<PackedInt2>().value();
                return true;


            default:
                return ApplyStandardExtendedProperty(index, data, propLen);
        }
    }

    bool ApplyStandardExtendedProperty(unsigned int index, ByteStream& data, unsigned int propLen) {
        switch (index) {
            case PROP_SHAPE_YOFFSET:
                m_actor->shape.yOffset = data.Read<PackedFloat4>().value();
                return true;
            case PROP_SCALE:
                m_actor->scale.x = data.Read<PackedFloat4>().value();
                m_actor->scale.y = data.Read<PackedFloat4>().value();
                m_actor->scale.z = data.Read<PackedFloat4>().value();
                return true;

            case PROP_FLAGS:
                m_actor->flags = data.Read<PackedUInt4>().value();
                return true;

            case PROP_VELOCITY:
                m_actor->velocity.x = data.Read<PackedFloat4>().value();
                m_actor->velocity.y = data.Read<PackedFloat4>().value();
                m_actor->velocity.z = data.Read<PackedFloat4>().value();
                m_actor->speedXZ = data.Read<PackedFloat4>().value();
                return true;

            case PROP_GRAVITY:
                m_actor->gravity = data.Read<PackedFloat4>().value();
                return true;

            case PROP_COLOR_FILTER:
                m_actor->colorFilterParams = data.Read<PackedUInt2>().value();
                m_actor->colorFilterTimer = data.Read<PackedUInt1>().value();
                return true;

            default:
                return false;
        }
    }

    virtual void BuildCustomProperties(ByteStream& out) {
    }

    virtual bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) {
        return false;
    }

    virtual void OnPropertiesApplied(u64 changed) {
    }

    static void ReadBlob(ByteStream& data, void* dst, unsigned int size) {
        unsigned int n = data.BytesLeft() < size ? data.BytesLeft() : size;
        data.Read(reinterpret_cast<char*>(dst), n);
    }

    ByteStream BuildAnimProperty(u8 animIndex, SkelAnime* skel) {
        return ByteStream() << PackedUInt1(animIndex) << PackedFloat4(skel->playSpeed)
                            << PackedInt2((s16)(skel->startFrame)) << PackedInt2((s16)(skel->endFrame))
                            << PackedFloat4(skel->animLength) << PackedUInt1(skel->mode);
    }

    void ApplyAnimProperty(void* anim, SkelAnime* skel, f32 fallbackCurFrame, ByteStream& data);

    ByteStream BuildAnimStateProperty(SkelAnime* skel, bool lockCurFrame) {
        return ByteStream() << PackedFloat4(lockCurFrame ? skel->curFrame : 0.0f) << PackedFloat4(skel->playSpeed)
                            << PackedInt2((s16)(skel->startFrame)) << PackedInt2((s16)(skel->endFrame))
                            << PackedFloat4(skel->animLength) << PackedUInt1(skel->mode);
    }

    void ApplyAnimStateProperty(SkelAnime* skel, ByteStream& data, bool lockCurFrame) {
        f32 frame = data.Read<PackedFloat4>().value();
        skel->curFrame = lockCurFrame ? frame : skel->curFrame;
        skel->playSpeed = data.Read<PackedFloat4>().value();
        skel->startFrame = static_cast<f32>(data.Read<PackedInt2>().value());
        skel->endFrame = static_cast<f32>(data.Read<PackedInt2>().value());
        skel->animLength = data.Read<PackedFloat4>().value();
        skel->mode = (u8)(data.Read<PackedUInt1>().value());
    }

    void SetAnimation(SkelAnime* skel, void* anim) {
        skel->animation = anim;
    }

    void RequestFullPropertySend() {
        m_forcePropResend = true;
    }

    bool EndConversation(PlayState* play);

    void ReinstallUpdate() {
        if (m_actor->update != nullptr && m_actor->update != &AbstractActorController::DispatchUpdate) {
            m_originalUpdate = m_actor->update;
            m_actor->update = &AbstractActorController::DispatchUpdate;
        }
    }

    template <class T> static ByteStream& PackProperty(unsigned int index, const T& value, ByteStream& out) {
        static_assert(IsPacked<T>::value, "PackProperty requires a Packed* type");
        out.WriteVarUInt(index);
        out.WriteVarUInt(sizeof(value));
        out << value;
        return out;
    }

    static ByteStream& PackProperty(unsigned int index, const ByteStream& data, ByteStream& out) {
        out.WriteVarUInt(index);
        out.WriteVarUInt(data.Length());
        out.Write(data);
        return out;
    }

    static ByteStream& PackNullProperty(unsigned int index, ByteStream& out) {
        out.WriteVarUInt(index);
        out.WriteVarUInt(0U);
        return out;
    }

    void WriteProperties(ByteStream& out) {
        ByteStream full;

        BuildStandardProperties(full);
        BuildCustomProperties(full);

        unsigned int propertyCount = 0;
        out.Write(DiffProperties(m_lastBuiltProperties, full, m_forcePropResend, &propertyCount));
        m_forcePropResend = false;
        m_lastBuiltProperties = full;
    }

    virtual void InitActorHealth() {
    }

    virtual void UpdatePuppet(PlayState* play) {
    }

    virtual void OnBecomeLeader() {
    }
    virtual void OnLoseLeadership() {
    }

    Actor* m_actor;

    PosRot m_spawnPosRot{};

    int m_networkID;
    int m_sceneKey;
    int m_roomIndex;
    bool m_isLeader;
    bool m_creator = false;
    ActorFunc m_originalInit;
    ActorFunc m_originalUpdate;
    ActorFunc m_originalDestroy;
    bool m_runningLocally = false;
    ByteStream m_pendingProperties;
    bool m_locked = false;
    bool m_lockSent = false;
    bool m_conversationHandled = false;
    ByteStream m_lastBuiltProperties;

    bool m_forcePropResend = true;

    int m_claimCooldown = 0;

    inline static AbstractActorController* s_currentLeaderContext = nullptr;
};

} // namespace ZeldaOnline
#endif
#endif