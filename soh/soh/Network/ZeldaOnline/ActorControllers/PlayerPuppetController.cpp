#include "PlayerPuppetController.hpp"
#include "../ZeldaOnlineClient.hpp"
#include "../HorsePuppet.hpp"
#include "../Packet.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "src/overlays/actors/ovl_En_Boom/z_en_boom.h"

extern Color_RGB8 sTunicColors[];
}

namespace ZeldaOnline {

static ColliderQuadInit sPuppetSwordQuadInit =
{
    {
        COLTYPE_METAL,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0xFFCFFFFF, 0x00, 0x00 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_NEAREST,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } },
};

static void BoomerangPuppet_Update(Actor* actor, PlayState* play) {
    (void)play;
    EnBoom* boom = reinterpret_cast<EnBoom*>(actor);

    boom->activeTimer++;

}

void PlayerPuppetController::BuildLocalPlayerProperties(Player* player, Actor* horseActor, ByteStream& out) {
    PackProperty(PROP_POS_X, PackedFloat4(player->actor.world.pos.x), out);
    PackProperty(PROP_POS_Y, PackedFloat4(player->actor.world.pos.y), out);
    PackProperty(PROP_POS_Z, PackedFloat4(player->actor.world.pos.z), out);
    PackProperty(PROP_ROT_X, PackedInt2(player->actor.world.rot.x), out);
    PackProperty(PROP_ROT_Y, PackedInt2(player->actor.world.rot.y), out);
    PackProperty(PROP_ROT_Z, PackedInt2(player->actor.world.rot.z), out);

    PackProperty(PROP_SHAPE_ROT, ByteStream() << PackedInt2(player->actor.shape.rot.x)
                                                     << PackedInt2(player->actor.shape.rot.y)
                                            << PackedInt2(player->actor.shape.rot.z), out);

    u8 onPlatform = 0;
    if ((player->actor.floorBgId != BGCHECK_SCENE) && (player->actor.bgCheckFlags & 1)) {
        DynaPolyActor* floorDyna = DynaPoly_GetActor(&gPlayState->colCtx, player->actor.floorBgId);
        if (floorDyna != NULL && floorDyna->actor.zoController != NULL) {
            onPlatform = 1;
        }
    }
    PackProperty(PPROP_ON_PLATFORM, PackedUInt1(onPlatform), out);

    PackProperty(PPROP_MOVEMENT_FLAGS, PackedUInt1(player->skelAnime.movementFlags), out);
    PackProperty(PPROP_REAL_ROOM_INDEX, PackedInt2(gPlayState->roomCtx.curRoom.num), out);

    PackProperty(PPROP_PREV_TRANSL, ByteStream() << PackedInt2(player->skelAnime.prevTransl.x)
                                                        << PackedInt2(player->skelAnime.prevTransl.y)
                                                        << PackedInt2(player->skelAnime.prevTransl.z), out);
    PackProperty(PPROP_UPPER_LIMB_ROT, ByteStream() << PackedInt2(player->upperLimbRot.x)
                                                           << PackedInt2(player->upperLimbRot.y)
                                                           << PackedInt2(player->upperLimbRot.z), out);

    PackProperty(PPROP_UNK_3BC,
                 ByteStream() << PackedInt2(player->unk_3BC.x) << PackedInt2(player->unk_3BC.y)
                              << PackedInt2(player->unk_3BC.z), out);

    {
        ByteStream pose;
        for (int i = 0; i < 24; i++) {
            pose << PackedInt2(player->skelAnime.jointTable[i].x) << PackedInt2(player->skelAnime.jointTable[i].y)
                 << PackedInt2(player->skelAnime.jointTable[i].z);
        }
        PackProperty(PPROP_POSE, pose, out);
    }
    PackProperty(PPROP_STATE_FLAGS1, PackedUInt4(player->stateFlags1), out);
    PackProperty(PPROP_STATE_FLAGS2, PackedUInt4(player->stateFlags2), out);
    PackProperty(PPROP_ITEM_ACTION, PackedInt1(player->itemAction), out);
    PackProperty(PPROP_HELD_ITEM_ACTION, PackedInt1(player->heldItemAction), out);
    PackProperty(PPROP_MODEL_GROUP, ByteStream() << PackedUInt1((u8)(player->modelGroup)), out);
    PackProperty(PPROP_LEFT_HAND_TYPE, ByteStream() << PackedUInt1((u8)(player->leftHandType)), out);
    PackProperty(PPROP_RIGHT_HAND_TYPE, ByteStream() << PackedUInt1((u8)(player->rightHandType)), out);
    PackProperty(PPROP_SHEATH_TYPE, ByteStream() << PackedUInt1((u8)(player->sheathType)), out);
    PackProperty(PPROP_INVINCIBILITY, ByteStream() << PackedInt1((s8)(player->invincibilityTimer)), out);
    PackProperty(PPROP_HELD_GET_ITEM, PackedInt2(player->unk_862), out);
    PackProperty(PPROP_CS_ACTION, ByteStream() << PackedUInt1((u8)(player->csAction)), out);
    PackProperty(PPROP_HELD_ITEM_SCALE, PackedFloat4(player->unk_85C), out);
    PackProperty(PPROP_STICK_FLAME_TIMER, PackedInt2(player->unk_860), out);

    int tunic = TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC));
    Color_RGB8 sTemp;
    Color_RGB8* color = &sTunicColors[tunic];
    if (tunic == PLAYER_TUNIC_KOKIRI && CVarGetInteger(CVAR_COSMETIC("Link.KokiriTunic.Changed"), 0)) {
        sTemp = CVarGetColor24(CVAR_COSMETIC("Link.KokiriTunic.Value"), sTunicColors[PLAYER_TUNIC_KOKIRI]);
        color = &sTemp;
    } else if (tunic == PLAYER_TUNIC_GORON && CVarGetInteger(CVAR_COSMETIC("Link.GoronTunic.Changed"), 0)) {
        sTemp = CVarGetColor24(CVAR_COSMETIC("Link.GoronTunic.Value"), sTunicColors[PLAYER_TUNIC_GORON]);
        color = &sTemp;
    } else if (tunic == PLAYER_TUNIC_ZORA && CVarGetInteger(CVAR_COSMETIC("Link.ZoraTunic.Changed"), 0)) {
        sTemp = CVarGetColor24(CVAR_COSMETIC("Link.ZoraTunic.Value"), sTunicColors[PLAYER_TUNIC_ZORA]);
        color = &sTemp;
    }

    PackProperty(PPROP_TUNIC_COLOUR,
                 ByteStream() << PackedUInt1(color->r) << PackedUInt1(color->g) << PackedUInt1(color->b), out);
    u8 present = (horseActor != NULL) ? 1u : 0u;

    u8 mounted = 0, animId = 0, animFrame = 0;
    f32 px = 0, py = 0, pz = 0;
    s16 rx = 0, ry = 0, rz = 0;
    if (horseActor != NULL) {
        EnHorse* horse = (EnHorse*)horseActor;
        mounted = (player->stateFlags1 & PLAYER_STATE1_ON_HORSE) && player->rideActor == horseActor;
        animId = (u8)(horse->animationIdx);
        animFrame = (u8)(horse->skin.skelAnime.curFrame);
        if (mounted) {
            px = horse->actor.world.pos.x - player->actor.world.pos.x;
            py = horse->actor.world.pos.y - player->actor.world.pos.y;
            pz = horse->actor.world.pos.z - player->actor.world.pos.z;
        } else {
            px = horse->actor.world.pos.x;
            py = horse->actor.world.pos.y;
            pz = horse->actor.world.pos.z;
        }
        rx = horse->actor.shape.rot.x;
        ry = horse->actor.shape.rot.y;
        rz = horse->actor.shape.rot.z;
    }
    PackProperty(PPROP_HORSE_PRESENT, PackedUInt1(present), out);
    PackProperty(PPROP_HORSE_MOUNTED, PackedUInt1(mounted), out);
    PackProperty(PPROP_HORSE_ANIM, PackedUInt1(animId), out);
    PackProperty(PPROP_HORSE_FRAME, PackedUInt1(animFrame), out);
    PackProperty(PPROP_HORSE_POS_X, PackedFloat4(px), out);
    PackProperty(PPROP_HORSE_POS_Y, PackedFloat4(py), out);
    PackProperty(PPROP_HORSE_POS_Z, PackedFloat4(pz), out);
    PackProperty(PPROP_HORSE_ROT_X, PackedInt2(rx), out);
    PackProperty(PPROP_HORSE_ROT_Y, PackedInt2(ry), out);
    PackProperty(PPROP_HORSE_ROT_Z, PackedInt2(rz), out);

    u32 heldId = 0;
    if (player->heldActor != nullptr && player->heldActor->zoController != nullptr)
        heldId = static_cast<AbstractActorController*>(player->heldActor->zoController)->NetworkID();

    PackProperty(PPROP_HELD_ACTOR, PackedUInt4(heldId), out);

    if ((player->meleeWeaponState > 0) &&
        ((player->meleeWeaponAnimation < 0x18) || (player->stateFlags2 & PLAYER_STATE2_SPIN_ATTACKING)) &&
        (player->meleeWeaponInfo[1].active)) {

        ByteStream payload;
        payload << PackedFloat4(player->meleeWeaponInfo[1].tip.x) << PackedFloat4(player->meleeWeaponInfo[1].tip.y) << PackedFloat4(player->meleeWeaponInfo[1].tip.z);
        payload << PackedFloat4(player->meleeWeaponInfo[1].base.x) << PackedFloat4(player->meleeWeaponInfo[1].base.y) << PackedFloat4(player->meleeWeaponInfo[1].base.z);
        payload << PackedUInt4(player->meleeWeaponQuads[0].info.toucher.dmgFlags);
        payload << PackedUInt1(player->meleeWeaponQuads[0].info.toucher.damage);
        PackProperty(PPROP_HITTING, payload, out);
    } else {
        PackNullProperty(PPROP_HITTING, out);
    }

    if ((player->stateFlags1 & PLAYER_STATE1_BOOMERANG_THROWN) && player->boomerangActor != nullptr) {
        Actor* boom = player->boomerangActor;
        ByteStream payload;
        payload << PackedFloat4(boom->world.pos.x) << PackedFloat4(boom->world.pos.y)
                << PackedFloat4(boom->world.pos.z);
        payload << PackedInt2(boom->shape.rot.x) << PackedInt2(boom->shape.rot.y) << PackedInt2(boom->shape.rot.z);
        PackProperty(PPROP_BOOMERANG, payload, out);
    } else {
        PackNullProperty(PPROP_BOOMERANG, out);
    }

}

void PlayerPuppetController::DrawSwordQuadDebug(PlayState* play)
{
    if (!m_hitting || !m_hasValidHitQuad) {
        return;
    }
    Vec3f* q = m_swordQuad.dim.quad;
    Collider_DrawPoly(play->state.gfxCtx, &q[2], &q[3], &q[1], 0, 255, 0);
    Collider_DrawPoly(play->state.gfxCtx, &q[1], &q[0], &q[2], 0, 255, 0);
}

void PlayerPuppetController::UpdatePuppet(PlayState* play) {
    auto player = reinterpret_cast<Player*>(m_actor);
    if (m_horse != nullptr) {
        if (m_state.horseMounted) {
            m_horse->world.pos.x = m_actor->world.pos.x + m_state.horsePos.x;
            m_horse->world.pos.y = m_actor->world.pos.y + m_state.horsePos.y;
            m_horse->world.pos.z = m_actor->world.pos.z + m_state.horsePos.z;
        } else {
            m_horse->world.pos.x = m_state.horsePos.x;
            m_horse->world.pos.y = m_state.horsePos.y;
            m_horse->world.pos.z = m_state.horsePos.z;
        }
        m_horse->shape.rot = m_state.horseRot;
        m_horse->world.rot = m_state.horseRot;
        HorsePuppet_SetAnimation(m_horse, m_state.horseAnimIndex);
        HorsePuppet_SetAnimFrame(m_horse, m_state.horseAnimFrame);
    }

    {
        Actor* want = nullptr;
        if (m_state.heldActorId) {
            auto controller = ZeldaOnlineClient::Instance->GetNetworkController(m_state.heldActorId);
            if (controller) {
                want = controller->GetActor();
            }
        }

        bool unresolved = (m_state.heldActorId != 0) && (want == nullptr);

        if (!unresolved && player->heldActor != want) {
            if (player->heldActor != nullptr) {
                if (player->heldActor->parent == m_actor) {
                    player->heldActor->parent = nullptr;
                }
                m_actor->child = nullptr;
                player->heldActor = nullptr;
                player->interactRangeActor = nullptr;
            }
            if (want != nullptr) {
                m_actor->child = want;
                want->parent = m_actor;
                player->heldActor = want;
                player->interactRangeActor = want;
            }
        }

    }
    m_originalUpdate(m_actor, play);

    UpdateSwordHitbox(play);

}

void PlayerPuppetController::KillHorse() {
    if (m_horse != nullptr) {
        m_horse->parent = nullptr;
        gZeldaOnlineEngineCleanup = true;
        Actor_Kill(m_horse);
        gZeldaOnlineEngineCleanup = false;
        m_horse = nullptr;
    }
}
void PlayerPuppetController::UpdateSwordHitbox(PlayState* play)
{
    if (!m_hitting) {
        if (m_swordQuad.base.actor != nullptr) {
            Collider_ResetQuadAT(play, &m_swordQuad.base);
            Collider_DestroyQuad(play, &m_swordQuad);
            m_swordQuad.base.actor = nullptr;
        }
        return;
    }

    if (!m_hasValidHitQuad)
        return;

    if (m_swordQuad.base.actor != nullptr) {
        CollisionCheck_SetAT(play, &play->colChkCtx, &m_swordQuad.base);
    }
}

bool PlayerPuppetController::ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) {
    switch (index) {
        case PPROP_MOVEMENT_FLAGS:
            m_state.movementFlags = (u8)(data.Read<PackedUInt1>().value());
            return true;

        case PPROP_ON_PLATFORM:
            m_state.onPlatform = (u8)(data.Read<PackedUInt1>().value());
            return true;

        case PPROP_REAL_ROOM_INDEX:
            m_realRoomIndex = (s16)(data.Read<PackedInt2>().value());
            return true;

        case PPROP_PREV_TRANSL:
            m_state.prevTransl.x = (s16)(data.Read<PackedInt2>().value());
            m_state.prevTransl.y = (s16)(data.Read<PackedInt2>().value());
            m_state.prevTransl.z = (s16)(data.Read<PackedInt2>().value());
            return true;
        case PPROP_UPPER_LIMB_ROT:
            m_state.upperLimbRot.x = (s16)(data.Read<PackedInt2>().value());
            m_state.upperLimbRot.y = (s16)(data.Read<PackedInt2>().value());
            m_state.upperLimbRot.z = (s16)(data.Read<PackedInt2>().value());
            return true;

        case PPROP_UNK_3BC:
            m_state.unk_3BC.x = (s16)(data.Read<PackedInt2>().value());
            m_state.unk_3BC.y = (s16)(data.Read<PackedInt2>().value());
            m_state.unk_3BC.z = (s16)(data.Read<PackedInt2>().value());
            return true;

        case PPROP_POSE:
            for (int i = 0; i < 24; i++) {
                m_state.jointTable[i].x = (s16)(data.Read<PackedInt2>().value());
                m_state.jointTable[i].y = (s16)(data.Read<PackedInt2>().value());
                m_state.jointTable[i].z = (s16)(data.Read<PackedInt2>().value());
            }
            return true;
        case PPROP_STATE_FLAGS1:
            m_state.stateFlags1 = data.Read<PackedUInt4>().value();
            return true;
        case PPROP_STATE_FLAGS2:
            m_state.stateFlags2 = data.Read<PackedUInt4>().value();
            return true;
        case PPROP_ITEM_ACTION:
            m_state.itemAction = (s8)(data.Read<PackedInt1>().value());
            return true;
        case PPROP_HELD_ITEM_ACTION:
            m_state.heldItemAction = (s8)(data.Read<PackedInt1>().value());
            return true;
        case PPROP_MODEL_GROUP:
            m_state.modelGroup = (s32)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_LEFT_HAND_TYPE:
            m_state.modelLeftHandType = (u8)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_RIGHT_HAND_TYPE:
            m_state.modelRightHandType = (u8)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_SHEATH_TYPE:
            m_state.modelSheathType = (u8)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_INVINCIBILITY:
            m_state.invincibilityTimer = (s8)(data.Read<PackedInt1>().value());
            return true;
        case PPROP_HELD_GET_ITEM:
            m_state.heldGetItemId = (s16)(data.Read<PackedInt2>().value());
            return true;
        case PPROP_CS_ACTION:
            m_state.csAction = (u8)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_HELD_ITEM_SCALE:
            m_state.unk_85C = data.Read<PackedFloat4>().value();
            return true;
        case PPROP_STICK_FLAME_TIMER:
            m_state.unk860 = (s16)(data.Read<PackedInt2>().value());
            return true;

        case PPROP_HORSE_PRESENT:
            m_pendingHorsePresent = (u8)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_HORSE_MOUNTED:
            m_state.horseMounted = (u8)(data.Read<PackedUInt1>().value()) & 1;
            return true;
        case PPROP_HORSE_ANIM:
            m_state.horseAnimIndex = (u8)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_HORSE_FRAME:
            m_state.horseAnimFrame = (u8)(data.Read<PackedUInt1>().value());
            return true;
        case PPROP_HORSE_POS_X:
            m_state.horsePos.x = data.Read<PackedFloat4>().value();
            return true;
        case PPROP_HORSE_POS_Y:
            m_state.horsePos.y = data.Read<PackedFloat4>().value();
            return true;
        case PPROP_HORSE_POS_Z:
            m_state.horsePos.z = data.Read<PackedFloat4>().value();
            return true;
        case PPROP_HORSE_ROT_X:
            m_state.horseRot.x = (s16)(data.Read<PackedInt2>().value());
            return true;
        case PPROP_HORSE_ROT_Y:
            m_state.horseRot.y = (s16)(data.Read<PackedInt2>().value());
            return true;
        case PPROP_HORSE_ROT_Z:
            m_state.horseRot.z = (s16)(data.Read<PackedInt2>().value());
            return true;

        case PPROP_HELD_ACTOR:
            m_state.heldActorId = data.Read<PackedUInt4>().value();
            return true;

         case PPROP_HITTING: {
            if (propLen == 0) {
                m_hitting = m_hasValidHitQuad = false;
            } else {
                if (m_hitting) {
                    m_meleePrevTip = m_hittingTip;
                    m_meleePrevBase = m_hittingBase;
                    m_hasValidHitQuad = true;
                } else {
                    m_hasValidHitQuad = false;
                }
                m_hitting = true;

                m_hittingTip.x = data.Read<PackedFloat4>().value();
                m_hittingTip.y = data.Read<PackedFloat4>().value();
                m_hittingTip.z = data.Read<PackedFloat4>().value();
                m_hittingBase.x = data.Read<PackedFloat4>().value();
                m_hittingBase.y = data.Read<PackedFloat4>().value();
                m_hittingBase.z = data.Read<PackedFloat4>().value();
                m_hittingDmgFlags = data.Read<PackedUInt4>().value();
                m_hittingDamage = data.Read<PackedUInt1>().value();

                if (m_hasValidHitQuad) {
                    if (m_swordQuad.base.actor == nullptr) {
                        Collider_InitQuad(gPlayState, &m_swordQuad);
                        Collider_SetQuad(gPlayState, &m_swordQuad, m_actor, &sPuppetSwordQuadInit);
                    }
                    m_swordQuad.info.toucher.dmgFlags = m_hittingDmgFlags;
                    m_swordQuad.info.toucher.damage = m_hittingDamage * 4;
                    m_swordQuad.base.atFlags = AT_ON | AT_TYPE_ENEMY;

                    Collider_SetQuadVertices(&m_swordQuad, &m_hittingBase, &m_hittingTip, &m_meleePrevBase,
                                             &m_meleePrevTip);
                }
            }
            return true;

        }

        case PPROP_BOOMERANG: {
            if (propLen == 0) {
                KillBoomerang();
                return true;
            }

            Vec3f pos;
            Vec3s rot;
            pos.x = data.Read<PackedFloat4>().value();
            pos.y = data.Read<PackedFloat4>().value();
            pos.z = data.Read<PackedFloat4>().value();
            rot.x = (s16)(data.Read<PackedInt2>().value());
            rot.y = (s16)(data.Read<PackedInt2>().value());
            rot.z = (s16)(data.Read<PackedInt2>().value());

            if (m_boomerang == nullptr) {

                auto boom = reinterpret_cast<EnBoom*>(Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, ACTOR_EN_BOOM,
                                                                     pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, pos.x,
                                                                     pos.y, pos.z, rot.x, rot.y, rot.z, 0, 0));
                if (boom != nullptr) {
                    boom->actor.update = BoomerangPuppet_Update;
                    boom->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
                    boom->actor.room = -1;
                    boom->collider.base.atFlags = AT_ON | AT_TYPE_ENEMY;
                    boom->collider.info.toucher.damage = 4;
                    m_boomerang = (Actor*)boom;
                }
            }

            if (m_boomerang != nullptr) {
                m_boomerang->world.pos = pos;
                m_boomerang->shape.rot = rot;
                m_boomerang->world.rot = rot;
            }
            return true;
        }

        case PPROP_TUNIC_COLOUR:
            m_tunicColour.r = data.Read<PackedUInt1>().value();
            m_tunicColour.g = data.Read<PackedUInt1>().value();
            m_tunicColour.b = data.Read<PackedUInt1>().value();
            return true;

        default:
            return false;
    }
}

void PlayerPuppetController::OnPropertiesApplied(u64 changed) {
    m_state.horseType = HORSE_EPONA;

    if (changed & (1ull << PPROP_HORSE_PRESENT)) {
        u8 riding = m_pendingHorsePresent ? 1 : 0;
        if (riding != m_state.riding) {
            m_state.riding = riding;
            if (riding && m_horse == nullptr) {
                f32 spawnX = m_state.horseMounted ? m_actor->world.pos.x + m_state.horsePos.x : (f32)m_state.horsePos.x;
                f32 spawnY = m_state.horseMounted ? m_actor->world.pos.y + m_state.horsePos.y : (f32)m_state.horsePos.y;
                f32 spawnZ = m_state.horseMounted ? m_actor->world.pos.z + m_state.horsePos.z : (f32)m_state.horsePos.z;

                s16 spawnParams = (s16)(ZO_HORSE_PUPPET | ((m_state.horseType == HORSE_HNI) ? 0x8000 : 0));

                m_horse = Actor_SpawnAsChildDirect(&gPlayState->actorCtx, m_actor, gPlayState, ACTOR_EN_HORSE, spawnX, spawnY, spawnZ,
                                            0, m_state.horseRot.y, 0, spawnX, spawnY, spawnZ, 0, m_state.horseRot.y, 0,
                                            spawnParams, 0);
            } else if (!riding && m_horse != nullptr) {
                KillHorse();
            }
        }
    }

    if (0) {
        Player* player = (Player*)m_actor;
        if ((player->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE)) && player->csAction != 0) {
            u32 h = (u32)m_networkID * 2654435761u;
            f32 angle = (h >> 16) * (2.0f * (f32)M_PI / 65536.0f);
            f32 radius = 40.0f + (h & 0x1F);
            m_actor->world.pos.x += cosf(angle) * radius;
            m_actor->world.pos.z += sinf(angle) * radius;
        }
    }
}
}
