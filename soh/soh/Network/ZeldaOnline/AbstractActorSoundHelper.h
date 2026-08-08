#ifndef ABSTRACT_ACTOR_SOUND_HELPER_H
#define ABSTRACT_ACTOR_SOUND_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

int ZeldaOnline_ShouldSuppressActorSound(unsigned short sfxId);
int ZeldaOnline_ShouldTransmitActorSound(unsigned short sfxId);

void ZeldaOnline_OnActorSound(unsigned short sfxId, float x, float y, float z);

#ifdef __cplusplus
}
#endif

#endif
