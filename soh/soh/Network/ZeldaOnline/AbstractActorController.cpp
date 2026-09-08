#include "AbstractActorController.hpp"
#include "ZeldaOnlineClient.hpp"

#include "PacketTypes.hpp"
#include "Packet.hpp"
#include <soh/ActorDB.h>
#include <spdlog/spdlog.h>




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

AbstractActorController::AbstractActorController(Actor* actor, int networkID, int sceneKey, int roomIndex,
                                                 bool isLeader)
    : m_actor(actor), m_networkID(networkID), m_sceneKey(sceneKey), m_roomIndex(roomIndex), m_isLeader(isLeader),
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

    if (m_originalInit) {
        s_currentLeaderContext = this;
        m_originalInit(m_actor, play);
        s_currentLeaderContext = nullptr;
        m_originalInit = nullptr;
    }

    OnActorInit();
    if (m_actor->update == nullptr)
        return;

    m_actor->flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    if (movedFromHome) {
        m_actor->world = m_spawnPosRot;
    }

    InitActorHealth();

    Math_Vec3f_Copy(&m_actor->prevPos, &m_actor->world.pos);

    ApplyPendingProperties();
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