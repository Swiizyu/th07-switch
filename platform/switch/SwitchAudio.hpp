// th07-switch: miniaudio -> SDL3 (libnx AUDOUT) audio bridge. See SwitchAudio.cpp.
#pragma once

#ifdef __SWITCH__

struct ma_engine;

namespace SwitchAudio
{
// Attach the (noDevice) miniaudio engine to an SDL3 playback stream.
bool Start(ma_engine *engine);

// Stop and release the playback stream.
void Stop();
} // namespace SwitchAudio

#endif
