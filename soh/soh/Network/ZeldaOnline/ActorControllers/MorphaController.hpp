#ifndef MORPHACONTROLLERH
#define MORPHACONTROLLERH

#include "../AbstractBossController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Mo/z_boss_mo.h"
#include "assets/textures/boss_title_cards/object_mo.h"
void BossMo_Tentacle(BossMo* mo, PlayState* play);
void BossMo_IntroCs(BossMo* mo, PlayState* play);

u16 func_800FA0B4(u8 seqPlayerIndex);
void func_80064534(PlayState* play, CutsceneContext* csCtx);
void BossMo_DeathCs(BossMo* mo, PlayState* play);
void BossMo_Core(BossMo* mo, PlayState* play);
void BossMo_CoreCollisionCheck(BossMo* mo, PlayState* play);

void BossMo_UpdateEffects(BossMo* mo, PlayState* play);

extern f32* gMorphaTentWidth;

extern BossMo* sMorphaCore;
extern BossMo* sMorphaTent1;
extern BossMo* sMorphaTent2;
}

namespace ZeldaOnline {

class MorphaController : public AbstractBossController {
    typedef enum {
        /*   0 */ MO_BATTLE,
        /*   1 */ MO_INTRO_WAIT,
        /*   2 */ MO_INTRO_START,
        /*   3 */ MO_INTRO_SWIM,
        /*   4 */ MO_INTRO_REVEAL,
        /*   5 */ MO_INTRO_FINISH,
        /* 100 */ MO_DEATH_START = 100,
        /* 101 */ MO_DEATH_DRAIN_WATER_1,
        /* 102 */ MO_DEATH_DRAIN_WATER_2,
        /* 103 */ MO_DEATH_CEILING,
        /* 104 */ MO_DEATH_DROPLET,
        /* 105 */ MO_DEATH_FINISH,
        /* 150 */ MO_DEATH_MO_CORE_BURST = 150
    } BossMoCsState;

    typedef enum {
        /*   0 */ MO_TENT_READY,
        /*   1 */ MO_TENT_SWING,
        /*   2 */ MO_TENT_ATTACK,
        /*   3 */ MO_TENT_CURL,
        /*   4 */ MO_TENT_GRAB,
        /*   5 */ MO_TENT_SHAKE,
        /*  10 */ MO_TENT_WAIT = 10,
        /*  11 */ MO_TENT_SPAWN,
        /* 100 */ MO_TENT_CUT = 100,
        /* 101 */ MO_TENT_RETREAT,
        /* 102 */ MO_TENT_DESPAWN,
        /* 200 */ MO_TENT_DEATH_START = 200,
        /* 201 */ MO_TENT_DEATH_1,
        /* 202 */ MO_TENT_DEATH_2,
        /* 203 */ MO_TENT_DEATH_3,
        /* 205 */ MO_TENT_DEATH_5 = 205,
        /* 206 */ MO_TENT_DEATH_6
    } BossMoTentState;

  public:
    using AbstractBossController::AbstractBossController;

    BossMo* Typed() const {
        return reinterpret_cast<BossMo*>(m_actor);
    }

    bool IsTentacle() const {
        return Typed()->actor.params >= BOSSMO_TENTACLE;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId != ACTOR_ITEM_B_HEART && actorId != ACTOR_DOOR_WARP1;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using MoActionFunc = decltype(&BossMo_Tentacle);
    static const MoActionFunc* ActionTable(size_t* count) {
        static const MoActionFunc sTable[] = {
            BossMo_Tentacle,
            BossMo_Core,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    const char* GetTitleCard() const override {
        return gMorphaTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return &Typed()->csCamera;
    }

    Vec3f_* GetCameraAt() override {
        return &Typed()->cameraAt;
    }

    Vec3f_* GetCameraEye() override {
        return &Typed()->cameraEye;
    }

    void OnActorInit() override {

        ReinstallUpdate();
        EndCutsceneCamera();

        if (Flags_GetClear(gPlayState, gPlayState->roomCtx.curRoom.num))
            Actor_Kill(m_actor);
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const MoActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    void DragOthers(PlayState* play) {
        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_BOSS].head; it != nullptr; it = it->next) {
            if (it == m_actor || it->id != ACTOR_BOSS_MO || it->zoController == nullptr)
                continue;

            auto* other = reinterpret_cast<AbstractActorController*>(it->zoController);
            if (!other->IsLeader())
                other->ClaimLeadership(CLAIM_REASON_NOW);
        }
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_WORK,
        PROP_TIMERS,
        PROP_FWORK,
        PROP_STATE,
        PROP_TENT_DRIVERS,
        PROP_TENT_ROT,
        PROP_TENT_STRETCH,
        PROP_TARGET_POS,
        PROP_WATER,
        PROP_CSSTATE,
        PROP_CS_CAMERA,
        PROP_BGM,
        PROP_TITLE_CARD
    };

    void BuildCustomProperties(ByteStream& out) override {
        BossMo* mo = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        ByteStream work;
        for (s32 i = 0; i < MO_SHORT_MAX; i++)
            work << PackedInt2(mo->work[i]);
        PackProperty(PROP_WORK, work, out);

        ByteStream timers;
        for (s32 i = 0; i < 5; i++)
            timers << PackedInt2(mo->timers[i]);
        PackProperty(PROP_TIMERS, timers, out);

        ByteStream fwork;
        for (s32 i = 0; i < MO_FLOAT_MAX; i++)
            fwork << PackedFloat4(mo->fwork[i]);
        PackProperty(PROP_FWORK, fwork, out);

        PackProperty(
            PROP_STATE,
            ByteStream() << PackedUInt1(mo->tent2KillTimer) << PackedUInt1(mo->hitCount)
                         << PackedUInt1(mo->tentSpawnPos) << PackedUInt1(mo->drawActor) << PackedUInt1(mo->linkHitTimer)
                         << PackedInt2(mo->cutIndex) << PackedInt2(mo->meltIndex) << PackedInt2(mo->linkToLeft)
                         << PackedInt2(mo->mashCounter) << PackedInt2(mo->noBubbles) << PackedInt2(mo->attackAngleMod)
                         << PackedInt2(mo->targetIndex) << PackedFloat4(mo->baseAlpha) << PackedFloat4(mo->cutScale)
                         << PackedUInt1(mo->actor.colChkInfo.health),
            out);


        PackProperty(PROP_TENT_DRIVERS,
                     ByteStream() << PackedInt2(mo->widthIndex) << PackedInt2(mo->pulsePhase) << PackedInt2(mo->xSwing)
                                  << PackedInt2(mo->zSwing) << PackedFloat4(mo->tentMaxAngle)
                                  << PackedFloat4(mo->tentSpeed) << PackedFloat4(mo->tentPulse)
                                  << PackedFloat4(mo->tentRippleSize) << PackedFloat4(mo->flattenRate),
                     out);


        if (IsTentacle()) {
            ByteStream rot;
            for (s32 i = 0; i < 41; i++)
                rot << PackedInt2(mo->tentRot[i].x) << PackedInt2(mo->tentRot[i].z);
            PackProperty(PROP_TENT_ROT, rot, out);
        } else {
            PackNullProperty(PROP_TENT_ROT, out);
        }


        if (IsTentacle()) {
            ByteStream stretch;
            for (s32 i = 0; i < 41; i++)
                stretch << PackedFloat4(mo->tentStretch[i].y);
            PackProperty(PROP_TENT_STRETCH, stretch, out);
        } else {
            PackNullProperty(PROP_TENT_STRETCH, out);
        }

        if (!IsTentacle()) {
            PackProperty(PROP_CSSTATE, PackedInt2(mo->csState), out);
        } else
            PackNullProperty(PROP_CSSTATE, out);

        PackProperty(PROP_TARGET_POS,
                     ByteStream() << PackedFloat4(mo->targetPos.x) << PackedFloat4(mo->targetPos.y)
                                  << PackedFloat4(mo->targetPos.z),
                     out);

        PackProperty(PROP_WATER,
                     ByteStream() << PackedFloat4(mo->waterLevel) << PackedFloat4(mo->waterLevelMod)
                                  << PackedFloat4(mo->waterTexAlpha),
                     out);

        if (!IsTentacle()) {
            BuildBossProperty(PROP_BOSS_CAMERA, out);
            BuildBossProperty(PROP_BOSS_BGM, out);
            BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
            BuildBossProperty(PROP_BOSS_LIGHTING, out);
        }



        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossMo* mo = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const MoActionFunc* table = ActionTable(&count);
                if (id < count)
                    mo->actionFunc = table[id];
                break;
            }

            case PROP_CSSTATE:
                if (propLen == 0)
                    break;
                mo->csState = (s16)data.Read<PackedInt2>().value();
                break;
            case PROP_WORK:
                for (s32 i = 0; i < MO_SHORT_MAX; i++)
                    mo->work[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 5; i++)
                    mo->timers[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FWORK:
                for (s32 i = 0; i < MO_FLOAT_MAX; i++)
                    mo->fwork[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_STATE:
                mo->tent2KillTimer = (u8)(data.Read<PackedUInt1>().value());
                mo->hitCount = (u8)(data.Read<PackedUInt1>().value());
                mo->tentSpawnPos = (u8)(data.Read<PackedUInt1>().value());
                mo->drawActor = (u8)(data.Read<PackedUInt1>().value());
                mo->linkHitTimer = (u8)(data.Read<PackedUInt1>().value());
                mo->cutIndex = (s16)(data.Read<PackedInt2>().value());
                mo->meltIndex = (s16)(data.Read<PackedInt2>().value());
                mo->linkToLeft = (s16)(data.Read<PackedInt2>().value());
                mo->mashCounter = (s16)(data.Read<PackedInt2>().value());
                mo->noBubbles = (s16)(data.Read<PackedInt2>().value());
                mo->attackAngleMod = (s16)(data.Read<PackedInt2>().value());
                mo->targetIndex = (s16)(data.Read<PackedInt2>().value());
                mo->baseAlpha = data.Read<PackedFloat4>().value();
                mo->cutScale = data.Read<PackedFloat4>().value();
                mo->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TENT_DRIVERS:
                mo->widthIndex = (s16)(data.Read<PackedInt2>().value());
                mo->pulsePhase = (s16)(data.Read<PackedInt2>().value());
                mo->xSwing = (s16)(data.Read<PackedInt2>().value());
                mo->zSwing = (s16)(data.Read<PackedInt2>().value());
                mo->tentMaxAngle = data.Read<PackedFloat4>().value();
                mo->tentSpeed = data.Read<PackedFloat4>().value();
                mo->tentPulse = data.Read<PackedFloat4>().value();
                mo->tentRippleSize = data.Read<PackedFloat4>().value();
                mo->flattenRate = data.Read<PackedFloat4>().value();
                break;
            case PROP_TENT_ROT:
                if (propLen == 0)
                    break;
                for (s32 i = 0; i < 41; i++) {
                    mo->tentRot[i].x = (s16)(data.Read<PackedInt2>().value());
                    mo->tentRot[i].z = (s16)(data.Read<PackedInt2>().value());
                }
                break;
            case PROP_TENT_STRETCH:
                if (propLen == 0)
                    break;
                for (s32 i = 0; i < 41; i++)
                    mo->tentStretch[i].y = data.Read<PackedFloat4>().value();
                break;
            case PROP_TARGET_POS:
                mo->targetPos.x = data.Read<PackedFloat4>().value();
                mo->targetPos.y = data.Read<PackedFloat4>().value();
                mo->targetPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_WATER:
                mo->waterLevel = data.Read<PackedFloat4>().value();
                mo->waterLevelMod = data.Read<PackedFloat4>().value();
                mo->waterTexAlpha = data.Read<PackedFloat4>().value();
                break;

            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }


    void OnPropertiesApplied(u64 changed) override {
        auto self = Typed();

        if (IsTentacle()) {
            if (self->work[MO_TENT_ACTION_STATE] >= MO_TENT_DEATH_START) {
                EndCutsceneCamera();
                Audio_QueueSeqCmd(0x1 << 28 | SEQ_PLAYER_BGM_MAIN << 24 | 0x100FF);
                GoLocal();
            }
        } else if (Typed()->csState >= MO_DEATH_START) {
            Player* player = GET_PLAYER(gPlayState);

            EndCutsceneCamera();
            Enemy_StartFinishingBlow(gPlayState, &self->actor);
            GameInteractor_ExecuteOnBossDefeat(&self->actor);
            self->csState = MO_DEATH_START;
            sMorphaTent1->drawActor = false;
            sMorphaTent1->work[MO_TENT_ACTION_STATE] = MO_TENT_DEATH_START;
            sMorphaTent1->baseAlpha = 0.0f;

            if (sMorphaTent2 != NULL) {
                sMorphaTent2->tent2KillTimer = 1;
            }
            if (player->actor.parent != NULL) {
                player->av2.actionVar2 = 0x65;
                player->actor.parent = NULL;
            }
            GoLocal();
        }
    }

    void OnBecomeLeader() override {
        BossMo* mo = Typed();

        if (!IsTentacle() || gPlayState == nullptr)
            return;

        s16 state = mo->work[MO_TENT_ACTION_STATE];
        if (state != MO_TENT_GRAB && state != MO_TENT_SHAKE)
            return;

        Player* player = GET_PLAYER(gPlayState);

        EndCutsceneCamera();

        if (player->actor.parent == &mo->actor) {
            player->av2.actionVar2 = 0x65;
            player->actor.parent = NULL;
            player->csAction = 0;
        }

        mo->work[MO_TENT_ACTION_STATE] = MO_TENT_RETREAT;
        mo->work[MO_TENT_INVINC_TIMER] = 50;
        mo->timers[0] = 75;
        mo->mashCounter = 0;
        mo->sfxTimer = 0;
        mo->tentMaxAngle = 0.5f;
        mo->tentSpeed = 480.0f;
        mo->fwork[MO_TENT_MAX_STRETCH] = 1.0f;
    }

    void RebuildLinks(PlayState* play) {
        BossMo* first = nullptr;
        BossMo* second = nullptr;

        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_BOSS].head; it != nullptr; it = it->next) {
            if (it->id != ACTOR_BOSS_MO)
                continue;

            BossMo* mo = reinterpret_cast<BossMo*>(it);
            if (mo->actor.params != BOSSMO_TENTACLE) {
                sMorphaCore = mo;
                continue;
            }

            if (first == nullptr)
                first = mo;
            else if (second == nullptr)
                second = mo;
        }

        sMorphaTent1 = first;
        sMorphaTent2 = second;

        if (first != nullptr && second != nullptr) {
            first->otherTent = &second->actor;
            second->otherTent = &first->actor;
        } else if (first != nullptr) {
            first->otherTent = nullptr;
        }
    }

    void UpdateLeader(PlayState* play) override {
        RebuildLinks(play);
        if (!IsTentacle())
            DragOthers(play);

        AbstractActorController::UpdateLeader(play);
        ReinstallUpdate();

        BossMo* mo = Typed();

        if (!IsTentacle()) {
            if (mo->csState >= MO_DEATH_START && !Flags_GetClear(play, play->roomCtx.curRoom.num))
                Flags_SetClear(play, play->roomCtx.curRoom.num);
        }
        
    }

    void UpdatePuppet(PlayState* play) override {
        BossMo* mo = Typed();
        RebuildLinks(play);
        // The effects are stepped from the update, not the draw.
        BossMo_UpdateEffects(mo, play);

        if (!IsTentacle()) {

            if (mo->csState == MO_INTRO_WAIT && IsLocalPlayerClosest())
            {
                if (ClaimLeadership(CLAIM_REASON_COOLDOWN)) {
                    UpdateLeader(play);
                    return;
                }
            }
            mo->actor.focus.pos = mo->actor.world.pos;
            mo->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
            if (mo->coreCollider.base.acFlags & AC_HIT) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }
            mo->coreCollider.base.acFlags &= ~AC_HIT;

            Collider_UpdateCylinder(&mo->actor, &mo->coreCollider);
            RegisterColliderBase(play, &mo->coreCollider.base, COLL_AC | COLL_AT | COLL_OC);
            return;
        }

        mo->tentCollider.base.acFlags &= ~AC_HIT;

        mo->widthIndex++;
        if (mo->widthIndex >= 300)
            mo->widthIndex = 0;

        mo->pulsePhase -= 3000;

        if (!m_widthSeeded) {
            for (s32 i = 0; i < 300; i++)
                mo->tentWidth[i] = 1.0f + mo->tentPulse;
            m_widthSeeded = true;
        }

        s16 index = mo->widthIndex;
        mo->tentWidth[index] = (Math_SinS(mo->pulsePhase) * mo->tentPulse) + (1.0f + mo->tentPulse);

        for (s32 i = 0; i < 41; i++) {
            index = ((mo->widthIndex - (i * 2)) + 300) % 300;
            Math_ApproachF(&mo->tentScale[i].x, mo->tentWidth[index] * gMorphaTentWidth[i], 0.5f, 0.3f);

            f32 ripple = Math_SinS((mo->work[MO_TENT_VAR_TIMER] * 12000) + (i * 20000));
            mo->tentRipple[i].x = (1.0f * ripple) * mo->tentRippleSize;

            mo->tentScale[i].y = mo->tentScale[i].z = mo->tentScale[i].x;
            mo->tentRipple[i].y = mo->tentRipple[i].z = mo->tentRipple[i].x;
        }

        RegisterColliderBase(play, &mo->tentCollider.base, COLL_AC | COLL_AT | COLL_OC);

        Player* localPlayer = GET_PLAYER(play);

        if ((localPlayer->stateFlags2 & PLAYER_STATE2_GRABBED_BY_ENEMY) && localPlayer->actor.parent == &mo->actor) {

            localPlayer->actor.parent = nullptr;
            localPlayer->stateFlags2 &= ~PLAYER_STATE2_GRABBED_BY_ENEMY;
            localPlayer->csAction = 0;
            localPlayer->av2.actionVar2 = 100;
            localPlayer->actor.shape.rot.x = 0;
            localPlayer->actor.shape.rot.z = 0;

            EndCutsceneCamera();
        }
    }
    bool m_widthSeeded = false;
};

} // namespace ZeldaOnline

#endif