#ifndef ANUBICETAGCONTROLLERH
#define ANUBICETAGCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Anubice_Tag/z_en_anubice_tag.h"

void EnAnubiceTag_SpawnAnubis(EnAnubiceTag* thisx, PlayState* play);
void EnAnubiceTag_ManageAnubis(EnAnubiceTag* thisx, PlayState* play);
}

namespace ZeldaOnline {

class AnubiceTagController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnAnubiceTag* Typed() const {
        return reinterpret_cast<EnAnubiceTag*>(m_actor);
    }

    void FollowAnubisLeadership() {
        if (!IsLeader())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using AnubiceTagActionFunc = void (*)(EnAnubiceTag*, PlayState*);
    static const AnubiceTagActionFunc* ActionTable(size_t* count) {
        static const AnubiceTagActionFunc sTable[] = {
            EnAnubiceTag_SpawnAnubis,
            EnAnubiceTag_ManageAnubis,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const AnubiceTagActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
    };

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnAnubiceTag* tag = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const AnubiceTagActionFunc* table = ActionTable(&count);
                if (id < count)
                    tag->actionFunc = table[id];
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void DriveMirror(PlayState* play) {
        EnAnubiceTag* tag = Typed();
        EnAnubice* anubis = tag->anubis;
        Vec3f offset;

        if (tag->actor.xzDistToPlayer < (200.0f + tag->triggerRange)) {
            if (!anubis->isLinkOutOfRange && !anubis->isKnockedback) {
                anubis->isMirroringLink = true;
                offset.x = -Math_SinS(tag->actor.yawTowardsPlayer) * tag->actor.xzDistToPlayer;
                offset.z = -Math_CosS(tag->actor.yawTowardsPlayer) * tag->actor.xzDistToPlayer;
                Math_ApproachF(&anubis->actor.world.pos.x, (tag->actor.world.pos.x + offset.x), 0.3f, 10.0f);
                Math_ApproachF(&anubis->actor.world.pos.z, (tag->actor.world.pos.z + offset.z), 0.3f, 10.0f);
            }
        } else if (anubis->isMirroringLink) {
            anubis->isLinkOutOfRange = true;
        }
    }

    void UpdateLeader(PlayState* play) override {
        EnAnubiceTag* tag = Typed();

        if (tag->actionFunc == EnAnubiceTag_SpawnAnubis) {
            if (tag->anubis != nullptr)
                tag->actionFunc = EnAnubiceTag_ManageAnubis;
            else
                EnAnubiceTag_SpawnAnubis(tag, play);
            return;
        }

        if (tag->anubis == nullptr) {
            Actor_Kill(&tag->actor);
            return;
        }

        if (tag->anubis->actor.update == nullptr)
            return;

        if (tag->anubis->deathTimer != 0) {
            Actor_Kill(&tag->actor);
            return;
        }

        DriveMirror(play);
    }

    void UpdatePuppet(PlayState* play) override {
    }
};

} // namespace ZeldaOnline

#endif