#ifndef POEPAINTINGCONTROLLERH
#define POEPAINTINGCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Po_Event/z_bg_po_event.h"
#include "objects/object_po_sisters/object_po_sisters.h"

void BgPoEvent_BlockWait(BgPoEvent* po, PlayState* play);
void BgPoEvent_BlockShake(BgPoEvent* po, PlayState* play);
void BgPoEvent_BlockFall(BgPoEvent* po, PlayState* play);
void BgPoEvent_BlockIdle(BgPoEvent* po, PlayState* play);
void BgPoEvent_BlockPush(BgPoEvent* po, PlayState* play);
void BgPoEvent_BlockReset(BgPoEvent* po, PlayState* play);
void BgPoEvent_BlockSolved(BgPoEvent* po, PlayState* play);
void BgPoEvent_AmyWait(BgPoEvent* po, PlayState* play);
void BgPoEvent_AmyPuzzle(BgPoEvent* po, PlayState* play);
void BgPoEvent_PaintingEmpty(BgPoEvent* po, PlayState* play);
void BgPoEvent_PaintingAppear(BgPoEvent* po, PlayState* play);
void BgPoEvent_PaintingPresent(BgPoEvent* po, PlayState* play);
void BgPoEvent_PaintingVanish(BgPoEvent* po, PlayState* play);
void BgPoEvent_PaintingBurn(BgPoEvent* po, PlayState* play);

extern ColliderTrisInit* gBgPoEventTrisInit;

extern u8 sBgPoEventBlocksAtRest;
extern u8 sBgPoEventPuzzleState;
extern f32 sBgPoEventblockPushDist;
}

namespace ZeldaOnline {

class PoePaintingController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    static void RegisterHooks(s16 actorID, bool enabled) {
        COND_ID_HOOK(ShouldActorInit, ACTOR_BG_PO_EVENT, enabled, [&](void* actorRef, bool*) {
            Actor* actor = (Actor*)actorRef;

            AbstractActorController::InstallCustomInit(actor, PoePaintingController::BgPoEvent_Init);
        });
    }

    BgPoEvent* Typed() const {
        return reinterpret_cast<BgPoEvent*>(m_actor);
    }


    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

    void OnActorInit() override {
        BgPoEvent* po = Typed();
        if (po->type == 2 || po->type == 3) {
            m_puzzleState = sBgPoEventPuzzleState;
        }
    }

    static void BgPoEvent_Init(Actor* thisx, PlayState* play) {
        static InitChainEntry sInitChain[] = {
            ICHAIN_VEC3F_DIV1000(scale, 1000, ICHAIN_STOP),
        };
        static s16 paintingPosX[] = { -1302, -866, 1421, 985 };
        static s16 paintingPosY[] = { 1107, 1091 };
        static s16 paintingPosZ[] = { -3384, -3252 };
        static s16 blockPosX[] = { 2149, 1969, 1909 };
        static s16 blockPosZ[] = { -1410, -1350, -1530 };

        BgPoEvent* po = reinterpret_cast<BgPoEvent*>(thisx);
        static int blockCount = 0;

        printf("PO OBJECT :%i:%i\n", thisx->params, ++blockCount);

        Actor_ProcessInitChain(thisx, sInitChain);
        po->type = (thisx->params >> 8) & 0xF;
        po->index = (thisx->params >> 0xC) & 0xF;
        thisx->params &= 0x3F;

        if (po->type < 2) {
            CollisionHeader* colHeader = NULL;
            s32 bgId;

            DynaPolyActor_Init(&po->dyna, DPM_UNK);
            if (Flags_GetSwitch(play, thisx->params)) {
                Actor_Kill(thisx);
                return;
            }

            po->dyna.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
            CollisionHeader_GetVirtual((void*)gPoSistersAmyBlockCol, &colHeader);
            po->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &po->dyna.actor, colHeader);

            if ((po->type == 0) && (po->index != 3)) {
                printf("SPAWNED: %p\n", Actor_SpawnAsChild(&play->actorCtx, &po->dyna.actor, play, ACTOR_BG_PO_EVENT, blockPosX[po->index],
                                   po->dyna.actor.world.pos.y, blockPosZ[po->index], 0, po->dyna.actor.shape.rot.y,
                                   po->dyna.actor.shape.rot.z - 0x4000,
                                   ((po->index + 1) << 0xC) + (po->type << 8) + po->dyna.actor.params));

                if ((po->index == 0) && (po->dyna.actor.child != NULL) && (po->dyna.actor.child->child != NULL) &&
                    (po->dyna.actor.child->child->child != NULL)) {
                    po->dyna.actor.parent = po->dyna.actor.child->child->child;
                    po->dyna.actor.child->child->child->child = &po->dyna.actor;
                }
            }

            po->dyna.actor.world.pos.y = 833.0f;
            po->dyna.actor.floorHeight = BgCheck_EntityRaycastFloor4(&play->colCtx, &po->dyna.actor.floorPoly, &bgId,
                                                                     &po->dyna.actor, &po->dyna.actor.world.pos);
            po->actionFunc = BgPoEvent_BlockWait;
            return;
        }

        Collider_InitTris(play, &po->collider);
        Collider_SetTris(play, &po->collider, thisx, gBgPoEventTrisInit, po->colliderItems);

        if (Flags_GetSwitch(play, thisx->params)) {
            Actor_Kill(thisx);
            return;
        }

        f32 sins = Math_SinS(po->dyna.actor.shape.rot.y);
        f32 coss = Math_CosS(po->dyna.actor.shape.rot.y);
        f32 scaleY = 1.0f;

        if (po->type == 4) {
            sins *= 2.4f;
            coss *= 2.4f;
            scaleY = 1.818f;
        }

        for (s32 i1 = 0; i1 < gBgPoEventTrisInit->count; i1++) {
            ColliderTrisElementInit* item = &gBgPoEventTrisInit->elements[i1];
            Vec3f v[3];
            for (s32 i2 = 0; i2 < 3; i2++) {
                Vec3f* vtxVec = &item->dim.vtx[i2];
                v[i2].x = (vtxVec->x * coss) + (po->dyna.actor.home.pos.x + (sins * vtxVec->z));
                v[i2].y = (vtxVec->y * scaleY) + po->dyna.actor.home.pos.y;
                v[i2].z = po->dyna.actor.home.pos.z + (coss * vtxVec->z) - (vtxVec->x * sins);
            }
            Collider_SetTrisVertices(&po->collider, i1, &v[0], &v[1], &v[2]);
        }

        if ((po->type != 4) && (po->index != 2)) {
            s32 phi_t2 = (po->type == 2) ? po->index : po->index + 2;
            Actor_SpawnAsChild(&play->actorCtx, &po->dyna.actor, play, ACTOR_BG_PO_EVENT, paintingPosX[phi_t2],
                               paintingPosY[po->index], paintingPosZ[po->index], 0, po->dyna.actor.shape.rot.y + 0x8000,
                               0, ((po->index + 1) << 0xC) + (po->type << 8) + po->dyna.actor.params);

            if ((po->index == 0) && (po->dyna.actor.child != NULL) && (po->dyna.actor.child->child != NULL)) {
                po->dyna.actor.parent = po->dyna.actor.child->child;
                po->dyna.actor.child->child->child = &po->dyna.actor;
            }
        }

        po->timer = 0;
        if (po->type == 4) {
            sBgPoEventPuzzleState = 0;
            po->actionFunc = BgPoEvent_AmyWait;
        } else {
            sBgPoEventPuzzleState = (s32)(Rand_ZeroOne() * 3.0f) % 3;
            po->actionFunc = BgPoEvent_PaintingEmpty;
        }
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using PoActionFunc = void (*)(BgPoEvent*, PlayState*);
    static const PoActionFunc* ActionTable(size_t* count) {
        static const PoActionFunc sTable[] = {
            BgPoEvent_BlockWait,      BgPoEvent_BlockShake,    BgPoEvent_BlockFall,      BgPoEvent_BlockIdle,
            BgPoEvent_BlockPush,      BgPoEvent_BlockReset,    BgPoEvent_BlockSolved,    BgPoEvent_AmyWait,
            BgPoEvent_AmyPuzzle,      BgPoEvent_PaintingEmpty, BgPoEvent_PaintingAppear, BgPoEvent_PaintingPresent,
            BgPoEvent_PaintingVanish, BgPoEvent_PaintingBurn,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const PoActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    void UpdateBurned() {
        if (Typed()->actionFunc == BgPoEvent_PaintingBurn)
            m_burned = true;
    }


    void RebuildLinks(PlayState* play) {
        BgPoEvent* po = Typed();
        UpdateBurned();


        if (po->type == 1 || po->type == 4 || m_burned)
            return;

        const u8 maxIndex = (po->type < 2) ? 3 : 2;
        if (po->index > maxIndex)
            return;

        BgPoEvent* nextAbove = nullptr; //lowest index above ours
        BgPoEvent* prevBelow = nullptr; //highest index below ours
        BgPoEvent* lowest = nullptr;
        BgPoEvent* highest = nullptr;
        s32 aliveCount = 1;

        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_BG].head; it != nullptr; it = it->next) {
            if (it == m_actor || it->id != ACTOR_BG_PO_EVENT || it->zoController == nullptr || it->update == nullptr)
                continue;

            BgPoEvent* other = reinterpret_cast<BgPoEvent*>(it);
            if (other->type != po->type || other->index > maxIndex)
                continue;

            auto* ctl = reinterpret_cast<PoePaintingController*>(it->zoController);
            ctl->UpdateBurned();
            if (ctl->m_burned)
                continue;

            aliveCount++;

            if (other->index > po->index && (nextAbove == nullptr || other->index < nextAbove->index))
                nextAbove = other;
            if (other->index < po->index && (prevBelow == nullptr || other->index > prevBelow->index))
                prevBelow = other;
            if (lowest == nullptr || other->index < lowest->index)
                lowest = other;
            if (highest == nullptr || other->index > highest->index)
                highest = other;
        }

        if (aliveCount == (maxIndex + 1))
            m_sawFullChain = true;
        if (!m_sawFullChain)
            return;

        BgPoEvent* child = (nextAbove != nullptr) ? nextAbove : lowest;
        BgPoEvent* parent = (prevBelow != nullptr) ? prevBelow : highest;

        po->dyna.actor.child = (child != nullptr) ? &child->dyna.actor : nullptr;
        po->dyna.actor.parent = (parent != nullptr) ? &parent->dyna.actor : nullptr;
    }

    void UpdateLeader(PlayState* play) override {
        RebuildLinks(play);


        BgPoEvent* po = Typed();
        if (po->type == 0 && po->dyna.actor.child == nullptr)
            return;

        auto oldPuzzleState = sBgPoEventPuzzleState;
        AbstractActorController::UpdateLeader(play);

        // This painting changed it
        if (oldPuzzleState != sBgPoEventPuzzleState)
            m_puzzleState = sBgPoEventPuzzleState;
    }

    bool IsPainting() const {
        return Typed()->type >= 2;
    }

    bool HitWouldReact() const {
        BgPoEvent* po = Typed();
        if (!IsPainting() || !(po->collider.base.acFlags & AC_HIT))
            return false;

        return po->actionFunc == BgPoEvent_PaintingPresent || po->actionFunc == BgPoEvent_AmyWait;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_SHARED,
        PROP_TIMER_SECONDS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgPoEvent* po = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(po->index) << PackedInt1(po->direction) << PackedInt2(po->timer), out);

        if (m_puzzleState == sBgPoEventPuzzleState)
            PackProperty(PROP_SHARED, PackedUInt1(m_puzzleState), out);
        else
            PackNullProperty(PROP_SHARED, out);

        if (po->type == 1)
            PackProperty(PROP_TIMER_SECONDS, PackedInt2(gSaveContext.timerSeconds), out);
        else
            PackNullProperty(PROP_TIMER_SECONDS, out);

    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgPoEvent* po = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const PoActionFunc* table = ActionTable(&count);
                if (id < count)
                    po->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                po->index = (u8)(data.Read<PackedUInt1>().value());
                po->direction = (s8)(data.Read<PackedInt1>().value());
                po->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SHARED:
                if (propLen != 0)
                    m_puzzleState = sBgPoEventPuzzleState = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_TIMER_SECONDS: {
                if (propLen == 0)
                    break;

                s16 seconds = (s16)(data.Read<PackedInt2>().value());

                if (seconds <= 0) {
                    gSaveContext.timerState = TIMER_STATE_OFF;
                } else {

                    s16 drift = (s16)(seconds - gSaveContext.timerSeconds);
                    if (gSaveContext.timerState == TIMER_STATE_OFF)
                        Interface_SetTimer(seconds);
                    else if (drift > 1 || drift < -1)
                        gSaveContext.timerSeconds = seconds;
                }
                
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgPoEvent* po = Typed();

        RebuildLinks(play);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        po->collider.base.acFlags &= ~AC_HIT;

        if (po->dyna.unk_150 != 0.0f) {
            ClaimLeadership(CLAIM_REASON_NOW);
            po->dyna.unk_150 = 0.0f;
        }

        if (IsPainting() && po->dyna.actor.xzDistToPlayer < 400.0f &&
            IsLocalPlayerClosest()) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
        }

        if (IsPainting()) {
            RegisterColliderBase(play, &po->collider.base, COLL_AC);
        }


        if (po->actionFunc == BgPoEvent_PaintingBurn) {
            static Vec3f sZeroVec = { 0, 0, 0 };
            Vec3f pos;
            pos.x = (Math_SinS(po->dyna.actor.shape.rot.y) * 5.0f) + po->dyna.actor.world.pos.x;
            pos.y = Rand_CenteredFloat(66.0f) + po->dyna.actor.world.pos.y;
            pos.z = Rand_CenteredFloat(50.0f) + po->dyna.actor.world.pos.z;

            if (po->timer >= 0) {
                if (po->type == 2) {
                    EffectSsDeadDb_Spawn(play, &pos, &sZeroVec, &sZeroVec, 100, 0, 255, 255, 150, 170, 255, 0, 0, 1, 9,
                                         true);
                } else {
                    EffectSsDeadDb_Spawn(play, &pos, &sZeroVec, &sZeroVec, 100, 0, 200, 255, 255, 170, 50, 100, 255, 1,
                                         9, true);
                }
            }

            if (po->timer == 0) {
                po->dyna.actor.draw = nullptr;
            }
        }
    }

  private:
    bool m_burned = false;
    bool m_sawFullChain = false;
    u8 m_puzzleState = 0xFF;
};

} // namespace ZeldaOnline

#endif