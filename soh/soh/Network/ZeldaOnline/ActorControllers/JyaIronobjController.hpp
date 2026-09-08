#ifndef JYAIRONOBJCONTROLLERH
#define JYAIRONOBJCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Jya_Ironobj/z_bg_jya_ironobj.h"

void func_808992E8(BgJyaIronobj*, PlayState* play);
typedef void (*BgJyaIronobjIkFunc)(BgJyaIronobj*, PlayState*, EnIk*);
void BgJyaIronobj_SpawnPillarParticles(BgJyaIronobj* thisx, PlayState* play, EnIk* enIk);
void BgJyaIronobj_SpawnThoneParticles(BgJyaIronobj* thisx, PlayState* play, EnIk* enIk);

}

namespace ZeldaOnline {

class JyaIronobjController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgJyaIronobj* Typed() const {
        return reinterpret_cast<BgJyaIronobj*>(m_actor);
    }

  protected:
    enum {
        PROP_COLL_ROLES = PROP_CUSTOM_START,
    };

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_COLL_ROLES, PackedUInt1(COLL_AC), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        switch (index) {
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgJyaIronobj* obj = Typed();


        if (obj->colCylinder.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        Collider_UpdateCylinder(&obj->dyna.actor, &obj->colCylinder);

        if (m_roles != 0)
            RegisterColliderBase(play, &obj->colCylinder.base, m_roles);
    }

    void OnServerDestroy() override {
        auto ikActor = Actor_Find(&gPlayState->actorCtx, ACTOR_EN_IK, ACTORCAT_BOSS);
        if (!ikActor)
            ikActor = Actor_Find(&gPlayState->actorCtx, ACTOR_EN_IK, ACTORCAT_ENEMY);
        
        if (ikActor) {
            static BgJyaIronobjIkFunc particleFunc[] = { BgJyaIronobj_SpawnPillarParticles,
                                                         BgJyaIronobj_SpawnThoneParticles };
            BgJyaIronobj* obj = Typed();
            particleFunc[obj->dyna.actor.params & 1](obj, gPlayState, (EnIk*)ikActor);
            SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &obj->dyna.actor.world.pos, 80,
                                               NA_SE_EN_IRONNACK_BREAK_PILLAR);

            Vec3f dropPos;
            dropPos.x = obj->dyna.actor.world.pos.x;
            dropPos.y = obj->dyna.actor.world.pos.y + 20.0f;
            dropPos.z = obj->dyna.actor.world.pos.z;
            for (int i = 0; i < 3; i++) {
                Item_DropCollectible(gPlayState, &dropPos, ITEM00_HEART);
                dropPos.y += 18.0f;
            }
        }
    }
    
  private:
    u8 m_roles = 0;
};

} // namespace ZeldaOnline

#endif
