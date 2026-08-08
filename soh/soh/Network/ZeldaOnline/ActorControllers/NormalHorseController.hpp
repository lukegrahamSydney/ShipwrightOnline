#ifndef NORMALHORSECONTROLLERH
#define NORMALHORSECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Horse_Normal/z_en_horse_normal.h"
#include "objects/object_horse_normal/object_horse_normal.h"
}

namespace ZeldaOnline {

static AnimationHeader* const s_horseNormalAnims[] = {
    (AnimationHeader*)&gHorseNormalIdleAnim,        (AnimationHeader*)&gHorseNormalWhinnyAnim,
    (AnimationHeader*)&gHorseNormalRefuseAnim,      (AnimationHeader*)&gHorseNormalRearingAnim,
    (AnimationHeader*)&gHorseNormalWalkingAnim,     (AnimationHeader*)&gHorseNormalTrottingAnim,
    (AnimationHeader*)&gHorseNormalGallopingAnim,   (AnimationHeader*)&gHorseNormalJumpingAnim,
    (AnimationHeader*)&gHorseNormalJumpingHighAnim,
};

class NormalHorseController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnHorseNormal* Horse() const {
        return reinterpret_cast<EnHorseNormal*>(m_actor);
    }

  protected:
    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ANIM_IDX,
        PROP_CUR_FRAME,
        PROP_WORLD_ROT_Y,
        PROP_WAYPOINT,
    };

    void OnActorInit() override {
        Horse()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnHorseNormal* horse = Horse();

        PackProperty(PROP_ACTION, ByteStream() << PackedUInt1((u8)(horse->action)), out);
        PackProperty(PROP_ANIM_IDX, ByteStream() << PackedUInt1((u8)(horse->animationIdx)), out);
        PackProperty(PROP_CUR_FRAME, PackedFloat4(horse->skin.skelAnime.curFrame), out);
        PackProperty(PROP_WORLD_ROT_Y, PackedInt2(horse->actor.world.rot.y), out);
        PackProperty(PROP_WAYPOINT, ByteStream() << PackedUInt1((u8)(horse->waypoint)), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnHorseNormal* horse = Horse();

        switch (index) {
            case PROP_ACTION:
                m_syncedAction = (u8)(data.Read<PackedUInt1>().value());
                Horse()->action = (s32)(m_syncedAction);
                break;

            case PROP_ANIM_IDX: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                if (id != m_syncedAnimId) {
                    m_syncedAnimId = id;
                    m_animDirty = true;
                }
                break;
            }
            case PROP_CUR_FRAME:
                m_syncedCurFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_WORLD_ROT_Y:
                horse->actor.world.rot.y = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_WAYPOINT:
                horse->waypoint = (s32)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

void OnPropertiesApplied(u64 changes) override {
        EnHorseNormal* horse = Horse();

        if ((changes & (1ull << PROP_ANIM_IDX)) && m_syncedAnimId != 0xFF) {
            AnimationHeader* anim = s_horseNormalAnims[m_syncedAnimId];
            Animation_Change(&horse->skin.skelAnime, anim, 1.0f, m_syncedCurFrame, Animation_GetLastFrame(anim),
                             ANIMMODE_LOOP, 0.0f);
            horse->animationIdx = m_syncedAnimId;
        } else {
            horse->skin.skelAnime.curFrame = m_syncedCurFrame;
        }
    }

    void OnBecomeLeader() override {
        if (m_syncedAction != 0xFF) {
            Horse()->action = (s32)(m_syncedAction);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnHorseNormal* horse = Horse();

        horse->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;

        horse->actor.shape.rot.y = horse->actor.world.rot.y;

        auto oldFrame = horse->skin.skelAnime.curFrame;
        SkelAnime_Update(&horse->skin.skelAnime);
        horse->skin.skelAnime.curFrame = oldFrame;

        horse->actor.focus.pos = horse->actor.world.pos;
        horse->actor.focus.pos.y += 70.0f;

        Collider_UpdateCylinder(&horse->actor, &horse->bodyCollider);
        CollisionCheck_SetOC(play, &play->colChkCtx, &horse->bodyCollider.base);
        horse->actor.colChkInfo.mass = (horse->actor.speedXZ == 0.0f) ? MASS_IMMOVABLE : MASS_HEAVY;
    }

  private:
    u8 m_syncedAction = 0xFF;
    u8 m_syncedAnimId = 0xFF;
    f32 m_syncedCurFrame = 0.0f;
    bool m_animDirty = false;
};

}

#endif
