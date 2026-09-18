#include "AbstractActorController.hpp"
#include "ZeldaOnlineClient.hpp"

#include "PacketTypes.hpp"
#include "Packet.hpp"
#include <soh/ActorDB.h>
#include <spdlog/spdlog.h>


extern "C" void EnSw_CrossProduct(Vec3f* a, Vec3f* b, Vec3f* dst);


namespace ZeldaOnline {
void AbstractActorController::SendUpdate() {
    ByteStream propUpdates;
    WriteProperties(propUpdates);

    if (propUpdates.Length() > 0) {
        ZeldaOnlineClient::Instance->WritePacket(newPacket(CLIENT_PACKET_ACTOR_PROPERTIES)
                                                 << PackedUInt2((unsigned int)(m_networkID)) << propUpdates);
    }
}

bool AbstractActorController::ClaimLeadership(u8 reason) {
    if (m_isLeader || (reason == CLAIM_REASON_COOLDOWN && m_claimCooldown > 0))
        return false;

    if (m_locked) {
        return false;
    }

    m_claimCooldown = 40;


    auto client = ZeldaOnlineClient::Instance;
    if (client == nullptr)
        return false;

    ByteStream packet = newPacket(CLIENT_PACKET_CLAIM_ACTOR);
    packet << PackedUInt2((u16)(m_networkID));
    packet << PackedUInt1(reason);
    client->WritePacket(packet);
    SetLeader(true);
    return true;
}

void AbstractActorController::SendTriggerToPuppets(const std::string& name, const ByteStream& data) {
    auto* client = ZeldaOnlineClient::Instance;
    if (client == nullptr || !client->isConnected || !m_isLeader) {
        return;
    }

    ByteStream packet = newPacket(CLIENT_PACKET_ACTOR_TRIGGER_TO_PUPPETS);
    packet << PackedUInt2((u16)(m_networkID));
    packet << PackedUInt1((unsigned int)(name.length()));
    packet << name;
    packet.Write(data);
    client->WritePacket(packet);
}

void AbstractActorController::SendTriggerToLeader(const std::string& name, const ByteStream& data) {
    auto* client = ZeldaOnlineClient::Instance;
    if (client == nullptr || !client->isConnected || m_isLeader) {
        return;
    }

    ByteStream packet = newPacket(CLIENT_PACKET_ACTOR_TRIGGER_TO_LEADER);
    packet << PackedUInt2((u16)(m_networkID));
    packet << PackedUInt1((unsigned int)(name.length()));
    packet << name;
    packet.Write(data);
    client->WritePacket(packet);
}

void AbstractActorController::UpdateLeadershipLock() {
    if (m_isLeader) {
        if (m_locked != ShouldLockActor()) {
            SendLeadershipLock(!m_locked);
        }
    }
}

void AbstractActorController::SendLeadershipLock(bool locked) {
    auto client = ZeldaOnlineClient::Instance;
    if (client == nullptr || !client->isConnected)
        return;
    m_locked = locked;
    ByteStream packet = newPacket(CLIENT_PACKET_SET_ACTOR_LOCKED);
    packet << PackedUInt2((u16)(m_networkID));
    packet << PackedUInt1(locked ? 1u : 0u);
    client->WritePacket(packet);
}

void AbstractActorController::GoLocal() {
    m_runningLocally = true;
    if (IsLeader()) {
        m_isLeader = false;
        auto client = ZeldaOnlineClient::Instance;
        if (client == nullptr || !client->isConnected)
            return;

        ByteStream p = newPacket(CLIENT_PACKET_RELINQUISH_ACTOR_LEADER);
        p << PackedUInt2(1) << PackedUInt2(NetworkID());
        client->WritePacket(p);
    }
}

bool AbstractActorController::UpdateAnimation(SkelAnime* skelAnime, bool lockFrame) {
    if (skelAnime == nullptr || skelAnime->update == nullptr || skelAnime->animation == nullptr ||
        skelAnime->skeleton == nullptr || skelAnime->jointTable == nullptr) {
        SPDLOG_WARN("[ZeldaOnline] skipping UpdateAnimation for actor {:#06x}: skeleton not ready",
                    m_actor != nullptr ? m_actor->id : 0);
        return false;
    }

    if (lockFrame) {
        auto playSpeed = skelAnime->playSpeed;
        skelAnime->playSpeed = 0.0f;
        bool retval = SkelAnime_Update(skelAnime) != 0;
        skelAnime->playSpeed = playSpeed;
        return retval;
    } else {
        return SkelAnime_Update(skelAnime) != 0;
    }
}

void AbstractActorController::RegisterCylinder(PlayState* play, ColliderCylinder* c, u8 roleBits) {
    Collider_UpdateCylinder(m_actor, c);
    if (roleBits & COLL_AT)
        CollisionCheck_SetAT(play, &play->colChkCtx, &c->base);
    if (roleBits & COLL_AC)
        CollisionCheck_SetAC(play, &play->colChkCtx, &c->base);
    if (roleBits & COLL_OC)
        CollisionCheck_SetOC(play, &play->colChkCtx, &c->base);
}

void AbstractActorController::RegisterColliderBase(PlayState* play, Collider* c, u8 roleBits) {
    if (roleBits & COLL_AT)
        CollisionCheck_SetAT(play, &play->colChkCtx, c);
    if (roleBits & COLL_AC)
        CollisionCheck_SetAC(play, &play->colChkCtx, c);
    if (roleBits & COLL_OC)
        CollisionCheck_SetOC(play, &play->colChkCtx, c);
}

bool AbstractActorController::IsLocalPlayerClosest() const {
    auto client = ZeldaOnlineClient::Instance;
    if (client == nullptr) {
        return true;
    }
    f32 localDistSq = SQ(m_actor->xzDistToPlayer);
    return client->IsLocalPlayerClosestTo(m_actor->world.pos, localDistSq);
}

void AbstractActorController::ApplyAnimProperty(void* anim, SkelAnime* skel, f32 fallbackCurFrame,
                                                       ByteStream& data) {
    bool changed = (anim != nullptr) && (skel->animation != anim);

    f32 playSpeed = data.Read<PackedFloat4>().value();
    f32 startFrame = static_cast<f32>(data.Read<PackedInt2>().value());
    f32 endFrame = static_cast<f32>(data.Read<PackedInt2>().value());
    f32 animLength = data.Read<PackedFloat4>().value();
    u8 mode = (u8)(data.Read<PackedUInt1>().value());

    if (changed) {

        Animation_ChangeImpl(skel, (AnimationHeader*)anim, playSpeed, startFrame, endFrame, mode, 0.0f, 0);
        skel->curFrame = fallbackCurFrame;
    } else {
        skel->playSpeed = playSpeed;
        skel->startFrame = startFrame;
        skel->endFrame = endFrame;
        skel->animLength = animLength;
        skel->mode = mode;
        SkelAnime_SetUpdate(skel);
    }
}

bool AbstractActorController::EndConversation(PlayState* play) {
    Player* player = GET_PLAYER(play);
    if (player == nullptr || player->talkActor != m_actor) {
        return false;
    }

    if (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) {
        Message_CloseTextbox(play);
    }
    m_actor->flags &= ~ACTOR_FLAG_TALK;
    player->stateFlags1 &= ~PLAYER_STATE1_TALKING;
    player->talkActor = nullptr;
    return true;
}

int AbstractActorController::RollNeighbourCount(float weight) const {
    return ZeldaOnlineClient::Instance != nullptr ? ZeldaOnlineClient::Instance->RollNeighbourCount(weight) : 0;
}

float AbstractActorController::RollEnemyHealthMultiplier(float weight) const {
    return ZeldaOnlineClient::Instance != nullptr ? ZeldaOnlineClient::Instance->RollEnemyHealthMultiplier(weight) : 0;
}

float AbstractActorController::RollBossHealthMultiplier(float weight) const {
    return ZeldaOnlineClient::Instance != nullptr ? ZeldaOnlineClient::Instance->RollBossHealthMultiplier(weight) : 0;
}

Actor* AbstractActorController::SpawnNeighbourActor(int actorID, const Vec3f& pos, const Vec3s& rot, int params) {
    return ZeldaOnlineClient::Instance->SpawnActor(actorID, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, params, true);
}

Actor* AbstractActorController::SpawnNeighbour(const Vec3f& pos, const Vec3s& rot) {
    return ZeldaOnlineClient::Instance->SpawnActor(m_actor->id, pos.x, pos.y, pos.z, rot.x, rot.y, rot.z,
                                                   m_originalParams, true);

}

void AbstractActorController::SpawnNeighboursGround(PlayState* play, float weight, f32 minRadius, f32 maxRadius) {
    int count = ZeldaOnlineClient::Instance->RollNeighbourCount(weight);
    for (int i = 0; i < count; i++) {
        Vec3f pos;

        if (!FindGroundSpawn(play, &pos, minRadius, maxRadius)) {
            continue;
        }

        SpawnNeighbour(pos, Vec3s{ m_actor->world.rot.x, m_actor->world.rot.y, m_actor->world.rot.z });
    }
}

void AbstractActorController::SpawnNeighboursWall(PlayState* play, f32 weight, const Vec3f& normal,
                                                         const Vec3f& tangentU, const Vec3f& tangentV, f32 minRadius,
                                                         f32 maxRadius)
{
    if (ZeldaOnlineClient::Instance == nullptr)
        return;

    int count = ZeldaOnlineClient::Instance->RollNeighbourCount(weight);

    for (int i = 0; i < count; i++) {
        Vec3f pos;
        Vec3s rot;

        if (!FindWallSpawn(play, normal, tangentU, tangentV, minRadius, maxRadius, &pos, &rot))
            continue;

        SpawnNeighbour(pos, rot);
    }
}

void AbstractActorController::SpawnNeighboursWater(PlayState* play, float weight, f32 minRadius, f32 maxRadius) {
    if (ZeldaOnlineClient::Instance == nullptr)
        return;

    int count = ZeldaOnlineClient::Instance->RollNeighbourCount(weight);

    for (int i = 0; i < count; i++) {
        Vec3f pos;

        if (!FindWaterSpawn(play, &pos, minRadius, maxRadius))
            continue;

        SpawnNeighbour(pos, m_actor->world.rot);
    }

}

void AbstractActorController::SpawnNeighboursAir(PlayState* play, float weight, f32 minRadius, f32 maxRadius) {
    if (ZeldaOnlineClient::Instance == nullptr)
        return;

    int count = ZeldaOnlineClient::Instance->RollNeighbourCount(weight);

    for (int i = 0; i < count; i++) {
        Vec3f pos;

        if (!FindAirSpawn(play, &pos, minRadius, maxRadius))
            continue;

        SpawnNeighbour(pos, m_actor->world.rot);
    }

}
bool AbstractActorController::FindGroundSpawn(PlayState* play, Vec3f* out, f32 minRadius, f32 maxRadius) {
    static constexpr int NEIGHBOUR_SPAWN_ATTEMPTS = 8;
    static constexpr f32 NEIGHBOUR_RAYCAST_HEIGHT = 300.0f;
    static constexpr f32 NEIGHBOUR_MAX_HEIGHT_DELTA = 40.0f;
    static constexpr f32 NEIGHBOUR_LINE_TEST_HEIGHT = 30.0f;
    static constexpr f32 NEIGHBOUR_CELL_SIZE = 100.0f;
    static constexpr f32 NEIGHBOUR_AIRBORNE_THRESHOLD = 80.0f;

    if (play == nullptr || m_actor == nullptr)
        return false;

    f32 cellSize = NEIGHBOUR_CELL_SIZE;

    if (cellSize > maxRadius)
        cellSize = maxRadius;

    if (cellSize <= 0.0f)
        return false;

    CollisionPoly* originPoly = NULL;
    s32 originBgId = 0;

    Vec3f originProbe = m_actor->world.pos;
    originProbe.y += NEIGHBOUR_RAYCAST_HEIGHT;

    f32 originFloorY = BgCheck_EntityRaycastFloor4(&play->colCtx, &originPoly, &originBgId, m_actor, &originProbe);

    if (originFloorY <= BGCHECK_Y_MIN)
        return false;

    const bool airborne = (m_actor->world.pos.y - originFloorY) > NEIGHBOUR_AIRBORNE_THRESHOLD;

    for (int attempt = 0; attempt < NEIGHBOUR_SPAWN_ATTEMPTS; attempt++) {
        s16 angle = (s16)(Rand_ZeroOne() * 65536.0f);
        f32 radius = minRadius + Rand_ZeroOne() * (maxRadius - minRadius);

        f32 offsetX = Math_SinS(angle) * radius;
        f32 offsetZ = Math_CosS(angle) * radius;

        offsetX = roundf(offsetX / cellSize) * cellSize;
        offsetZ = roundf(offsetZ / cellSize) * cellSize;

        if (offsetX == 0.0f && offsetZ == 0.0f)
            continue;

        Vec3f candidate;
        candidate.x = m_actor->world.pos.x + offsetX;
        candidate.y = originFloorY + NEIGHBOUR_RAYCAST_HEIGHT;
        candidate.z = m_actor->world.pos.z + offsetZ;

        CollisionPoly* poly = NULL;
        s32 bgId = 0;

        f32 floorY = BgCheck_EntityRaycastFloor4(&play->colCtx, &poly, &bgId, m_actor, &candidate);

        if (bgId != originBgId)
            continue;

        if (floorY <= BGCHECK_Y_MIN)
            continue;

        if (!airborne && fabsf(floorY - originFloorY) > NEIGHBOUR_MAX_HEIGHT_DELTA)
            continue;

        Vec3f from;
        from.x = m_actor->world.pos.x;
        from.y = originFloorY + NEIGHBOUR_LINE_TEST_HEIGHT;
        from.z = m_actor->world.pos.z;

        Vec3f to;
        to.x = candidate.x;
        to.y = floorY + NEIGHBOUR_LINE_TEST_HEIGHT;
        to.z = candidate.z;

        Vec3f hitPos;
        CollisionPoly* hitPoly = NULL;
        s32 hitBgId = 0;

        if (BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hitPos, &hitPoly, true, false, false, true, &hitBgId)) {
            continue;
        }

        out->x = candidate.x;
        out->y = airborne ? m_actor->world.pos.y : floorY;
        out->z = candidate.z;

        return true;
    }

    return false;
}


bool AbstractActorController::FindWallSpawn(PlayState* play, const Vec3f& normal, const Vec3f& tangentU,
                                            const Vec3f& tangentV, f32 minRadius, f32 maxRadius, Vec3f* outPos,
                                            Vec3s* outRot) {
    static constexpr int WALL_SPAWN_ATTEMPTS = 10;
    static constexpr f32 WALL_STEP_OUT = 30.0f;
    static constexpr f32 WALL_PROBE_DEPTH = 80.0f;
    static constexpr f32 WALL_SURFACE_OFFSET = 1.0f;
    static constexpr f32 WALL_NORMAL_TOLERANCE = 0.85f;

    if (play == nullptr || m_actor == nullptr)
        return false;

    for (int attempt = 0; attempt < WALL_SPAWN_ATTEMPTS; attempt++) {
        s16 angle = (s16)(Rand_ZeroOne() * 65536.0f);
        f32 radius = minRadius + Rand_ZeroOne() * (maxRadius - minRadius);

        f32 u = Math_SinS(angle) * radius;
        f32 v = Math_CosS(angle) * radius;

        Vec3f surfacePoint;
        surfacePoint.x = m_actor->world.pos.x + (tangentU.x * u) + (tangentV.x * v);
        surfacePoint.y = m_actor->world.pos.y + (tangentU.y * u) + (tangentV.y * v);
        surfacePoint.z = m_actor->world.pos.z + (tangentU.z * u) + (tangentV.z * v);

        Vec3f from;
        from.x = surfacePoint.x + (normal.x * WALL_STEP_OUT);
        from.y = surfacePoint.y + (normal.y * WALL_STEP_OUT);
        from.z = surfacePoint.z + (normal.z * WALL_STEP_OUT);

        Vec3f to;
        to.x = surfacePoint.x - (normal.x * WALL_PROBE_DEPTH);
        to.y = surfacePoint.y - (normal.y * WALL_PROBE_DEPTH);
        to.z = surfacePoint.z - (normal.z * WALL_PROBE_DEPTH);

        Vec3f hitPos;
        CollisionPoly* hitPoly = NULL;
        s32 hitBgId = 0;

        if (!BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hitPos, &hitPoly, true, true, true, false, &hitBgId)) {
            continue;
        }

        if (func_80041DB8(&play->colCtx, hitPoly, hitBgId) & 0x30)
            continue;

        if (SurfaceType_IsIgnoredByProjectiles(&play->colCtx, hitPoly, hitBgId))
            continue;

        Vec3f polyNormal;
        polyNormal.x = COLPOLY_GET_NORMAL(hitPoly->normal.x);
        polyNormal.y = COLPOLY_GET_NORMAL(hitPoly->normal.y);
        polyNormal.z = COLPOLY_GET_NORMAL(hitPoly->normal.z);

        if (DOTXYZ(polyNormal, normal) < WALL_NORMAL_TOLERANCE)
            continue;

        Vec3f originOut;
        originOut.x = m_actor->world.pos.x + (normal.x * WALL_STEP_OUT);
        originOut.y = m_actor->world.pos.y + (normal.y * WALL_STEP_OUT);
        originOut.z = m_actor->world.pos.z + (normal.z * WALL_STEP_OUT);

        Vec3f blocked;
        CollisionPoly* blockedPoly = NULL;
        s32 blockedBgId = 0;

        if (BgCheck_EntityLineTest1(&play->colCtx, &originOut, &from, &blocked, &blockedPoly, true, true, true, false,
                                    &blockedBgId)) {
            continue;
        }

        outPos->x = hitPos.x + (polyNormal.x * WALL_SURFACE_OFFSET);
        outPos->y = hitPos.y + (polyNormal.y * WALL_SURFACE_OFFSET);
        outPos->z = hitPos.z + (polyNormal.z * WALL_SURFACE_OFFSET);

        *outRot = m_actor->world.rot;

        return true;
    }

    return false;
}

bool AbstractActorController::FindAirSpawn(PlayState* play, Vec3f* out, f32 minRadius, f32 maxRadius) {
    static constexpr int AIR_SPAWN_ATTEMPTS = 10;
    static constexpr f32 AIR_VERTICAL_SPREAD = 40.0f;
    static constexpr f32 AIR_MIN_FLOOR_CLEARANCE = 20.0f;
    static constexpr f32 AIR_CEILING_CLEARANCE = 20.0f;
    static constexpr f32 AIR_RAYCAST_HEIGHT = 200.0f;

    if (play == nullptr || m_actor == nullptr)
        return false;

    for (int attempt = 0; attempt < AIR_SPAWN_ATTEMPTS; attempt++) {
        s16 angle = (s16)(Rand_ZeroOne() * 65536.0f);
        f32 radius = minRadius + Rand_ZeroOne() * (maxRadius - minRadius);
        f32 vertical = (Rand_ZeroOne() - 0.5f) * 2.0f * AIR_VERTICAL_SPREAD;

        Vec3f candidate;
        candidate.x = m_actor->world.pos.x + (Math_SinS(angle) * radius);
        candidate.y = m_actor->world.pos.y + vertical;
        candidate.z = m_actor->world.pos.z + (Math_CosS(angle) * radius);

        CollisionPoly* floorPoly = NULL;
        s32 floorBgId = 0;

        Vec3f floorProbe = candidate;
        floorProbe.y += AIR_RAYCAST_HEIGHT;

        f32 floorY = BgCheck_EntityRaycastFloor4(&play->colCtx, &floorPoly, &floorBgId, m_actor, &floorProbe);

        if (floorY <= BGCHECK_Y_MIN)
            continue;

        if (candidate.y - floorY < AIR_MIN_FLOOR_CLEARANCE)
            continue;

        Vec3f ceilingHit;
        CollisionPoly* ceilingPoly = NULL;
        s32 ceilingBgId = 0;

        Vec3f ceilingFrom = candidate;
        Vec3f ceilingTo = candidate;
        ceilingTo.y += AIR_CEILING_CLEARANCE;

        if (BgCheck_EntityLineTest1(&play->colCtx, &ceilingFrom, &ceilingTo, &ceilingHit, &ceilingPoly, false, false,
                                    true, false, &ceilingBgId)) {
            continue;
        }

        Vec3f hitPos;
        CollisionPoly* hitPoly = NULL;
        s32 hitBgId = 0;

        if (BgCheck_EntityLineTest1(&play->colCtx, &m_actor->world.pos, &candidate, &hitPos, &hitPoly, true, true, true,
                                    false, &hitBgId)) {
            continue;
        }

        *out = candidate;
        return true;
    }

    return false;
}

bool AbstractActorController::FindWaterSpawn(PlayState* play, Vec3f* out, f32 minRadius, f32 maxRadius) {
    static constexpr int WATER_SPAWN_ATTEMPTS = 10;
    static constexpr f32 WATER_CELL_SIZE = 100.0f;

    if (play == nullptr || m_actor == nullptr)
        return false;

    f32 cellSize = WATER_CELL_SIZE;

    if (cellSize > maxRadius)
        cellSize = maxRadius;

    if (cellSize <= 0.0f)
        return false;

    for (int attempt = 0; attempt < WATER_SPAWN_ATTEMPTS; attempt++) {
        s16 angle = (s16)(Rand_ZeroOne() * 65536.0f);
        f32 radius = minRadius + Rand_ZeroOne() * (maxRadius - minRadius);

        f32 offsetX = Math_SinS(angle) * radius;
        f32 offsetZ = Math_CosS(angle) * radius;

        offsetX = roundf(offsetX / cellSize) * cellSize;
        offsetZ = roundf(offsetZ / cellSize) * cellSize;

        if (offsetX == 0.0f && offsetZ == 0.0f)
            continue;

        Vec3f candidate;
        candidate.x = m_actor->world.pos.x + offsetX;
        candidate.y = m_actor->world.pos.y;
        candidate.z = m_actor->world.pos.z + offsetZ;

        WaterBox* waterBox = NULL;
        f32 waterSurface = candidate.y;

        // if (!WaterBox_GetSurface1(play, &play->colCtx, candidate.x, candidate.z, &waterSurface, &waterBox))
        //     continue;

        Vec3f hitPos;
        CollisionPoly* hitPoly = NULL;
        s32 hitBgId = 0;

        if (BgCheck_EntityLineTest1(&play->colCtx, &m_actor->world.pos, &candidate, &hitPos, &hitPoly, true, true, true,
                                    false, &hitBgId)) {
            continue;
        }

        *out = candidate;
        return true;
    }

    return false;
}
AbstractActorController::AbstractActorController(Actor* actor, int networkID, int sceneKey, int roomIndex,
                                                 bool isLeader)
    : m_actor(actor), m_originalParams(m_actor->params), m_networkID(networkID), m_sceneKey(sceneKey), m_roomIndex(roomIndex),
      m_isLeader(isLeader),
      m_originalInit(actor->init), m_originalUpdate(actor->update), m_originalDestroy(actor->destroy),
      m_startedAsLeader (isLeader){
    m_actor->zoController = this;

    m_spawnPosRot = actor->world;

    if (actor->id == ACTOR_BOSS_VA) {
        printf("BREAK\n");
    }

    if (m_originalInit)
        m_actor->init = DispatchInit;
    m_actor->update = AbstractActorController::DispatchUpdate;
    m_actor->destroy = AbstractActorController::DispatchDestroy;
}

void AbstractActorController::DispatchDestroy(Actor* actor, PlayState* play) {
    auto* controller = static_cast<AbstractActorController*>(actor->zoController);
    if (controller == nullptr) {
        if (actor->destroy != nullptr && actor->destroy != DispatchDestroy) {
            actor->destroy(actor, play);
        }
        return;
    }

    ActorFunc original = controller->m_originalDestroy;

    actor->zoController = nullptr;
    controller->m_actor = nullptr;

    ZeldaOnlineClient::Instance->RemoveNetworkedActor(controller->NetworkID(), controller);
    delete controller;

    if (original != NULL) {
        original(actor, play);
    }
}

void AbstractActorController::ActorInit(PlayState* play) {
    const bool movedFromHome = m_spawnPosRot.pos.x != m_actor->home.pos.x ||
                               m_spawnPosRot.pos.y != m_actor->home.pos.y || m_spawnPosRot.pos.z != m_actor->home.pos.z;
    s_currentLeaderContext = this;
    if (m_originalInit) {
        
        m_originalInit(m_actor, play);
        
        m_originalInit = nullptr;
    }

    OnActorInit();
    s_currentLeaderContext = nullptr;
    if (m_actor->update == nullptr)
        return;

    m_actor->flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    if (movedFromHome) {
        m_actor->world = m_spawnPosRot;
    }

    s_currentLeaderContext = this;
    InitActorHealth();
    s_currentLeaderContext = nullptr;
    Math_Vec3f_Copy(&m_actor->prevPos, &m_actor->world.pos);

    ApplyPendingProperties();

    if (IsCreator() && CanNeighbourSpawn())
    {
        printf("SPAWNING NEIGHBOURS\n");

        s_currentLeaderContext = this;
        SpawnNeighbours(play);
        s_currentLeaderContext = nullptr;
        //spawn neighbour enemies to make game harder. use party size
    }
}


void AbstractActorController::Update(PlayState* play) {
    if (m_runningLocally) {
        m_originalUpdate(m_actor, play);
        return;
    }
    UpdateLeadershipLock();

    if (m_isLeader) {

        if (m_actor->init == nullptr) {
            s_actorSoundContext = s_currentLeaderContext = this;
            UpdateLeader(play);
            s_actorSoundContext = s_currentLeaderContext = nullptr;

            // Do not send updates if we've been killed. Our actor_kill hook will already send updates BEFORE the kill
            // packet
            if (m_actor->update)
                SendUpdate();
        }
    } else {

        if (m_actor->init == nullptr) {
            ApplyPendingProperties();

            m_conversationHandled = false;
            s_actorSoundContext = this;
            UpdatePuppet(play);
            s_actorSoundContext = nullptr;

            //We became leader during the puppet execution
            if (m_isLeader) {
            //Send any updates
                if (m_actor->update)
                    SendUpdate();
            } else if (!m_conversationHandled) {
                EndConversation(play);
            }
        }
    }
    if (m_claimCooldown > 0)
        m_claimCooldown--;
}
} // namespace ZeldaOnline