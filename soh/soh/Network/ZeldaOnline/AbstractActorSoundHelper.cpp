#include <unordered_map>
#include "AbstractActorSoundHelper.h"
#include "AbstractActorController.hpp"
#include "ZeldaOnlineClient.hpp"

extern "C" int ZeldaOnline_ShouldSuppressActorSound(unsigned short sfxId) {
    auto* ctx = ZeldaOnline::AbstractActorController::s_actorSoundContext;
    return (ctx && ctx->ShouldSuppressSounds()) ? 1 : 0;
}

extern "C" int ZeldaOnline_ShouldTransmitActorSound(unsigned short sfxId) {
    auto* ctx = ZeldaOnline::AbstractActorController::s_actorSoundContext;
    if (!ctx || !ctx->ShouldTransmitSounds()) {
        return false;
    }

    static std::unordered_map<u16, u32> s_lastForwardFrame;

    u32 frame = gPlayState ? gPlayState->gameplayFrames : 0;
    auto it = s_lastForwardFrame.find(sfxId);
    if (it != s_lastForwardFrame.end() && it->second == frame) {
        return false;
    }
    s_lastForwardFrame[sfxId] = frame;
    return true;
}

extern "C" void ZeldaOnline_OnActorSound(unsigned short sfxId, float x, float y, float z) {
    int networkID = 0;
    auto* ctx = ZeldaOnline::AbstractActorController::s_actorSoundContext;
    if (ctx != nullptr) {
        networkID = ctx->NetworkID();
    }
    ZeldaOnline::ZeldaOnlineClient::Instance->TransmitActorSound(networkID, sfxId);
}
