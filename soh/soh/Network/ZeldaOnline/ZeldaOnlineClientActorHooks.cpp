#include "ZeldaOnlineClient.hpp"

#include "ZeldaOnlineClient.hpp"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "ActorControllers/PlayerPuppetController.hpp"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "src/overlays/actors/ovl_Bg_Ydan_Sp/z_bg_ydan_sp.h"
#include "src/overlays/actors/ovl_Obj_Switch/z_obj_switch.h"
#include "src/overlays/actors/ovl_En_Kusa/z_en_kusa.h"
#include "src/overlays/actors/ovl_En_Kanban/z_en_kanban.h"
#include "src/overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "src/overlays/actors/ovl_Bg_Ddan_Kd/z_bg_ddan_kd.h"
#include "src/overlays/actors/ovl_Bg_Dodoago/z_bg_dodoago.h"
#include "src/overlays/actors/ovl_Obj_Bombiwa/z_obj_bombiwa.h"
#include "src/overlays/actors/ovl_Obj_Hamishi/z_obj_hamishi.h"
#include "src/overlays/actors/ovl_Bg_Breakwall/z_bg_breakwall.h"
#include "src/overlays/actors/ovl_Bg_Bombwall/z_bg_bombwall.h"
#include "src/overlays/actors/ovl_Bg_Spot16_Bombstone/z_bg_spot16_bombstone.h"
#include "src/overlays/actors/ovl_Bg_Ydan_Maruta/z_bg_ydan_maruta.h"
#include "src/overlays/actors/ovl_En_Bx/z_en_bx.h"
#include "src/overlays/actors/ovl_En_Ba/z_en_ba.h"
#include "src/overlays/actors/ovl_Door_Shutter/z_door_shutter.h"
#include "src/overlays/actors/ovl_Bg_Bdan_Switch/z_bg_bdan_switch.h"
#include "src/overlays/actors/ovl_Bg_Spot02_Objects/z_bg_spot02_objects.h"

extern PlayState* gPlayState;

void BgYdanSp_FloorWebIdle(BgYdanSp* web, PlayState* play);
void BgYdanSp_WallWebIdle(BgYdanSp* web, PlayState* play);
void BgYdanSp_BurnWeb(BgYdanSp* web, PlayState* play);
void BgYdanSp_FloorWebBreaking(BgYdanSp* thisx, PlayState* play);
void ObjSwitch_EyeOpen(ObjSwitch* , PlayState* play);
void ObjSwitch_EyeClosingInit(ObjSwitch*);
void ObjSwitch_EyeClosed(ObjSwitch*, PlayState* play);
void ObjSwitch_EyeOpeningInit(ObjSwitch*);

void EnKusa_CutWaitRegrow(EnKusa*, PlayState* play);
void BgDdanKd_LowerStairs(BgDdanKd*, PlayState* play);

#include "src/overlays/actors/ovl_Bg_Dodoago/z_bg_dodoago.h"
void BgDodoago_WaitExplosives(BgDodoago* self, PlayState* play);
void BgDodoago_LightOneEye(BgDodoago* self, PlayState* play);
void BgDodoago_OpenJaw(BgDodoago* self, PlayState* play);
void BgDodoago_DoNothing(BgDodoago* self, PlayState* play);
void ObjBombiwa_Break(ObjBombiwa* objBombiwa, PlayState* play);
void ObjHamishi_Break(ObjHamishi* objHamishi, PlayState* play);
void BgBreakwall_Wait(BgBreakwall* bgBreakwall, PlayState* play);
void func_8086ED70(BgBombwall* bgBombwall, PlayState* play);
void func_808B5950(BgSpot16Bombstone* , PlayState* play);
void func_808BF078(BgYdanMaruta* thisx, PlayState* play);
void EnArrow_Fly(EnArrow*, PlayState* play);
s32 DoorShutter_GetPlayerSide(DoorShutter* thisx, PlayState* play);
void DoorShutter_Idle(DoorShutter* thisx, PlayState* play);
void DoorShutter_UnbarredCheckSwitchFlag(DoorShutter* thisx, PlayState* play);
void DoorShutter_Unbar(DoorShutter* thisx, PlayState* play);
void DoorShutter_BarAndWaitSwitchFlag(DoorShutter* thisx, PlayState* play);
void DoorShutter_WaitPlayerSurprised(DoorShutter* thiss, PlayState* play);
void DoorShutter_SetupAction(DoorShutter* thixs, DoorShutterActionFunc actionFunc);
void func_8086D730(BgBdanSwitch* sw);
void DoorShutter_Idle(DoorShutter* thisx, PlayState* play);
void func_808ACA08(BgSpot02Objects* thisx, PlayState* play);
void func_808AC908(BgSpot02Objects* thisx, PlayState* play);
}

namespace ZeldaOnline {
void ZeldaOnlineClient::RegisterActorHooks(bool enabled) {

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_YDAN_SP, enabled, [&](void* refActor, bool* should) {
        BgYdanSp* web = static_cast<BgYdanSp*>(refActor);
        if ((web->actionFunc == BgYdanSp_FloorWebIdle || web->actionFunc == BgYdanSp_WallWebIdle) &&
            Flags_GetSwitch(gPlayState, web->isDestroyedSwitchFlag)) {

            if (web->actionFunc == BgYdanSp_FloorWebIdle) {
                web->unk_16C = 200.0f;
                web->dyna.actor.room = -1;
                web->dyna.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
                web->timer = 40;
                Audio_PlayActorSound2(&web->dyna.actor, NA_SE_EV_WEB_BROKEN);
                web->actionFunc = BgYdanSp_FloorWebBreaking;
            } else
                BgYdanSp_BurnWeb(web, gPlayState);
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_OBJ_SWITCH, enabled, [&](void* refActor, bool* should) {
        ObjSwitch* sw = static_cast<ObjSwitch*>(refActor);
        bool flagSet = Flags_GetSwitch(gPlayState, (sw->dyna.actor.params >> 8) & 0x3F);
        if (sw->actionFunc == ObjSwitch_EyeOpen && flagSet) {
            ObjSwitch_EyeClosingInit(sw);
        } else if (sw->actionFunc == ObjSwitch_EyeClosed && !flagSet) {
            ObjSwitch_EyeOpeningInit(sw);
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_EN_KANBAN, enabled, [&](void* refActor, bool*) {
        EnKanban* kanban = static_cast<EnKanban*>(refActor);
        if (kanban->collider.base.acFlags & AC_HIT) {
            if (kanban->collider.base.acFlags & AC_TYPE_OTHER) {
                kanban->collider.base.acFlags &= ~AC_TYPE_OTHER;
            } else {
                SendHitTrigger(ACTOR_EN_KANBAN, kanban->actor.world.pos.x, kanban->actor.world.pos.y,
                               kanban->actor.world.pos.z);
            }
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_DDAN_KD, enabled, [&](void* refActor, bool*) {
        BgDdanKd* self = static_cast<BgDdanKd*>(refActor);

        if (self->actionFunc == BgDdanKd_LowerStairs && !(self->dyna.actor.flags & ACTOR_FLAG_ZO_USER2)) {
            self->dyna.actor.flags |= ACTOR_FLAG_ZO_USER2;
            SendSceneTrigger("dodongo_lowerStairs", ByteStream());
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_DODOAGO, enabled, [&](void* refActor, bool*) {
        BgDodoago* self = static_cast<BgDodoago*>(refActor);

        if (self->actionFunc == BgDodoago_LightOneEye && !(self->dyna.actor.flags & ACTOR_FLAG_ZO_USER1)) {
            self->dyna.actor.flags |= ACTOR_FLAG_ZO_USER1;
            ByteStream data;
            data << PackedInt2(self->state);
            SendSceneTrigger("dodongo_lightEye", data);
        }

        if (self->actionFunc == BgDodoago_OpenJaw && !(self->dyna.actor.flags & ACTOR_FLAG_ZO_USER2)) {
            self->dyna.actor.flags |= ACTOR_FLAG_ZO_USER2;
            SendSceneTrigger("dodongo_openJaw", ByteStream());
        }
    });

    COND_ID_HOOK(ShouldActorInit, ACTOR_EN_ARROW, enabled, [&](void* refActor, bool*) {
        EnArrow* arrow = static_cast<EnArrow*>(refActor);
        if (m_applyingRemoteSpawn)
            return;

        if (arrow->actor.params != ARROW_NUT)
            return;

        ByteStream p;
        p << PackedFloat4(arrow->actor.world.pos.x);
        p << PackedFloat4(arrow->actor.world.pos.y);
        p << PackedFloat4(arrow->actor.world.pos.z);
        p << PackedInt2(arrow->actor.world.rot.x);
        p << PackedInt2(arrow->actor.world.rot.y);
        p << PackedInt2(arrow->actor.params);
        p << PackedFloat4(arrow->actor.speedXZ);
        p << PackedFloat4(arrow->actor.velocity.y);
        SendSceneTrigger("arrow", p);
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_EN_ARROW, enabled, [&](void* refActor, bool*) {
        EnArrow* arrow = static_cast<EnArrow*>(refActor);
        if (m_applyingRemoteSpawn)
            return;

        if (arrow->actor.params == ARROW_NUT)
            return;

        bool launchedNow = (arrow->actor.parent == NULL) && (arrow->actionFunc == EnArrow_Fly) &&
                           !(arrow->actor.flags & ACTOR_FLAG_ZO_USER1) && !(arrow->actor.flags & ACTOR_FLAG_ZO_USER2);
        if (!launchedNow)
            return;

        arrow->actor.flags |= ACTOR_FLAG_ZO_USER2;

        ByteStream p;
        p << PackedFloat4(arrow->actor.world.pos.x);
        p << PackedFloat4(arrow->actor.world.pos.y);
        p << PackedFloat4(arrow->actor.world.pos.z);
        p << PackedInt2(arrow->actor.world.rot.x);
        p << PackedInt2(arrow->actor.world.rot.y);
        p << PackedInt2(arrow->actor.params);
        p << PackedFloat4(arrow->actor.speedXZ);
        p << PackedFloat4(arrow->actor.velocity.y);
        SendSceneTrigger("arrow", p);
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_YDAN_MARUTA, enabled, [&](void* refActor, bool* should) {
        BgYdanMaruta* ladder = static_cast<BgYdanMaruta*>(refActor);
        if (ladder->dyna.actor.params == 1 && ladder->actionFunc == func_808BF078 &&
            Flags_GetSwitch(gPlayState, ladder->switchFlag)) {
            ladder->collider.base.acFlags |= AC_HIT;
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_BREAKWALL, enabled, [&](void* refActor, bool* should) {
        BgBreakwall* actor = static_cast<BgBreakwall*>(refActor);

        if (actor->actionFunc == BgBreakwall_Wait && Flags_GetSwitch(gPlayState, actor->dyna.actor.params & 0x3F)) {
            actor->collider.base.acFlags |= AC_HIT;
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_OBJ_BOMBIWA, enabled, [&](void* refActor, bool* should) {
        ObjBombiwa* actor = static_cast<ObjBombiwa*>(refActor);

        if (Flags_GetSwitch(gPlayState, actor->actor.params & 0x3F)) {
            ObjBombiwa_Break(actor, gPlayState);
            SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &actor->actor.world.pos, 80, NA_SE_EV_WALL_BROKEN);
            Actor_Kill(&actor->actor);
            *should = false;
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_OBJ_HAMISHI, enabled, [&](void* refActor, bool* should) {
        ObjHamishi* actor = static_cast<ObjHamishi*>(refActor);

        if (Flags_GetSwitch(gPlayState, actor->actor.params & 0x3F)) {
            ObjHamishi_Break(actor, gPlayState);
            SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &actor->actor.world.pos, 40, NA_SE_EV_WALL_BROKEN);
            Actor_Kill(&actor->actor);
            *should = false;
        }
    });

    COND_ID_HOOK(ShouldActorInit, ACTOR_EN_BX, enabled, [&](void* refActor, bool* should) {
        EnBx* actor = static_cast<EnBx*>(refActor);
        *(s16*)actor->unk_150 = (actor->actor.params >> 8) & 0xFF;
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_EN_BX, enabled, [&](void* refActor, bool* should) {
        EnBx* actor = static_cast<EnBx*>(refActor);

        if (Flags_GetSwitch(gPlayState, *(s16*)actor->unk_150)) {
            for (int i = 0; i < 4; i++) {
                Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_BA, actor->unk_154[i].x, actor->unk_154[i].y,
                            actor->unk_154[i].z, 0, 0, 0, EN_BA_DEAD_BLOB);
            }
            SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &actor->actor.world.pos, 40, NA_SE_EN_BALINADE_HAND_DEAD);

            Actor_Kill(&actor->actor);
            *should = false;
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_BOMBWALL, enabled, [&](void* refActor, bool* should) {
        BgBombwall* actor = static_cast<BgBombwall*>(refActor);

        if (actor->actionFunc == func_8086ED70 && Flags_GetSwitch(gPlayState, actor->dyna.actor.params & 0x3F)) {
            actor->collider.base.acFlags |= AC_HIT;
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_SPOT16_BOMBSTONE, enabled, [&](void* refActor, bool* should) {
        BgSpot16Bombstone* actor = static_cast<BgSpot16Bombstone*>(refActor);

        if (actor->actionFunc == func_808B5950 && Flags_GetSwitch(gPlayState, actor->switchFlag)) {
            actor->colliderCylinder.base.acFlags |= AC_HIT;
        }
    });

    COND_VB_SHOULD(VB_PLAY_ONEPOINT_ACTOR_CS, enabled, {
        DoorShutter* actor = va_arg(args, DoorShutter*);

        if (actor->dyna.actor.id != ACTOR_DOOR_SHUTTER)
            return;

        u8 doorType = (actor->dyna.actor.params >> 6) & 0xF;
        if (gPlayState->sceneNum == SCENE_JABU_JABU &&
            (doorType == SHUTTER_FRONT_SWITCH || doorType == SHUTTER_FRONT_SWITCH_BACK_CLEAR)) {
            *should = false;
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_BDAN_SWITCH, enabled, [&](void* refActor, bool* should) {
        BgBdanSwitch* sw = static_cast<BgBdanSwitch*>(refActor);

        if (sw->dyna.actor.flags & ACTOR_FLAG_ZO_USER1) {
            return;
        }

        if (!(GET_PLAYER(gPlayState)->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE)) {
            sw->dyna.actor.flags |= ACTOR_FLAG_ZO_USER1;
            return;
        }

        bool pressed = false;
        switch (sw->dyna.actor.params & 0xFF) {
            case BLUE:
                pressed = DynaPolyActor_IsSwitchPressed(&sw->dyna);
                break;
            case YELLOW:
                pressed = DynaPolyActor_IsPlayerOnTop(&sw->dyna);
                break;
            default:
                break;
        }

        if (pressed) {
            func_8086D730(sw);
            s32 flag = (sw->dyna.actor.params >> 8) & 0x3F;

            Flags_SetSwitch(gPlayState, flag);

            sw->dyna.actor.flags |= ACTOR_FLAG_ZO_USER1;
            return;
        }
    });

    COND_ID_HOOK(ShouldActorUpdate, ACTOR_BG_SPOT02_OBJECTS, enabled, [&](void* refActor, bool* should) {
        BgSpot02Objects* grave = static_cast<BgSpot02Objects*>(refActor);

        if (grave->dyna.actor.params == 2) {
            if (grave->actionFunc == func_808ACA08 && (grave->dyna.actor.flags & ACTOR_FLAG_ZO_USER1) == 0) {
                grave->dyna.actor.flags |= ACTOR_FLAG_ZO_USER1;
                SendSceneTrigger("destroyroyalgrave", ByteStream());
            }
        }
    });

    COND_VB_SHOULD(VB_APPLY_TUNIC_COLOR, enabled, {
        if (gPlayState == nullptr)
            return;
        Actor* myPlayer = (Actor*)GET_PLAYER(gPlayState);
        Actor* actor = gPlayState->pauseCtx.state == 0 ? va_arg(args, Actor*) : myPlayer;

        Color_RGB8* color = va_arg(args, Color_RGB8*);
        if (actor) {

            if (actor == myPlayer) {
                return;
            }

            auto controller = static_cast<PlayerPuppetController*>(actor->zoController);
            if (controller)
                *color = controller->GetTunicColour();
        }
    });
}

void ZeldaOnlineClient::GetTunicColours(Player* player, Color_RGB8* out) {
}
} // namespace ZeldaOnline