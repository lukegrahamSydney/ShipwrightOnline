#ifndef BACONTROLLERH
#define BACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ba/z_en_ba.h"

void EnBa_Idle(EnBa* ba, PlayState* play);
void EnBa_FallAsBlob(EnBa* ba, PlayState* play);
void EnBa_SwingAtPlayer(EnBa* ba, PlayState* play);
void EnBa_RecoilFromDamage(EnBa* ba, PlayState* play);
void EnBa_Die(EnBa* ba, PlayState* play);
}

namespace ZeldaOnline {

class BaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBa* Typed() const {
        return reinterpret_cast<EnBa*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return (params & 0xFF) < EN_BA_DEAD_BLOB;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_DIE = 4;
    static constexpr int BLOB_FIRST_JOINT = 7;
    static constexpr int BLOB_LAST_JOINT = 13;

    using BaActionFunc = void (*)(EnBa*, PlayState*);
    static const BaActionFunc* ActionTable(size_t* count) {
        static const BaActionFunc sTable[] = {
            EnBa_Idle,
            EnBa_FallAsBlob,
            EnBa_SwingAtPlayer,
            EnBa_RecoilFromDamage,
            EnBa_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        return (Typed()->unk_14C >= 2) ? COLL_AC : 0;
    }

    bool HitWouldReact() const {
        EnBa* ba = Typed();
        return (ba->actor.params < EN_BA_DEAD_BLOB) && (ba->collider.base.acFlags & AC_HIT);
    }

    void SpawnDeathBlobs(PlayState* play) {
        EnBa* ba = Typed();
        for (int i = BLOB_FIRST_JOINT; i <= BLOB_LAST_JOINT; i++) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_EN_BA, ba->unk_158[i].x, ba->unk_158[i].y, ba->unk_158[i].z, 0, 0,
                        0, EN_BA_DEAD_BLOB);
        }
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_STATE,
        PROP_TARGET,
        PROP_SWAY,
        PROP_TIMERS,
        PROP_MASS,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBa* ba = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(ba->actor.colChkInfo.health), out);
        PackProperty(PROP_STATE, PackedInt4(ba->unk_14C), out);
        PackProperty(PROP_TARGET,
                     ByteStream() << PackedFloat4(ba->unk_2FC.x) << PackedFloat4(ba->unk_2FC.y)
                                  << PackedFloat4(ba->unk_2FC.z),
                     out);
        PackProperty(PROP_SWAY,
                     ByteStream() << PackedFloat4(ba->unk_308.x) << PackedFloat4(ba->unk_308.y)
                                  << PackedFloat4(ba->unk_308.z) << PackedFloat4(ba->unk_314),
                     out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(ba->unk_318) << PackedInt2(ba->unk_31A) << PackedInt2(ba->unk_31C),
                     out);
        PackProperty(PROP_MASS, PackedUInt1(ba->actor.colChkInfo.mass), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBa* ba = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const BaActionFunc* table = ActionTable(&count);
                if (id < count)
                    ba->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                ba->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_STATE:
                ba->unk_14C = data.Read<PackedInt4>().value();
                break;
            case PROP_TARGET:
                ba->unk_2FC.x = data.Read<PackedFloat4>().value();
                ba->unk_2FC.y = data.Read<PackedFloat4>().value();
                ba->unk_2FC.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_SWAY:
                ba->unk_308.x = data.Read<PackedFloat4>().value();
                ba->unk_308.y = data.Read<PackedFloat4>().value();
                ba->unk_308.z = data.Read<PackedFloat4>().value();
                ba->unk_314 = data.Read<PackedFloat4>().value();
                break;
            case PROP_TIMERS:
                ba->unk_318 = (s16)(data.Read<PackedInt2>().value());
                ba->unk_31A = (s16)(data.Read<PackedInt2>().value());
                ba->unk_31C = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_MASS:
                ba->actor.colChkInfo.mass = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
        if (Typed()->actionFunc == EnBa_Die)
            m_ranLocalDeath = true;
    }

    void OnServerDestroy() override {
        if (m_ranLocalDeath || gPlayState == nullptr)
            return;
        SpawnDeathBlobs(gPlayState);
    }

    void UpdatePuppet(PlayState* play) override {
        EnBa* ba = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        ba->collider.base.acFlags &= ~AC_HIT;
        ba->collider.base.atFlags &= ~AT_HIT;

        if (m_currentActionIndex != ID_DIE && ba->actor.xzDistToPlayer < 250.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        if (ba->actionFunc != nullptr)
            ba->actionFunc(ba, play);

        if (ba->actor.params < EN_BA_DEAD_BLOB)
            ba->actor.focus.pos = ba->unk_158[6];

        if (m_roles != 0)
            RegisterColliderBase(play, &ba->collider.base, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AC;
    bool m_ranLocalDeath = false;
};

}

#endif
