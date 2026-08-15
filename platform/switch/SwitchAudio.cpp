// th07-switch: audio output for Horizon OS.
//
// History of this file, because it explains the design:
//
//   v1: miniaudio engine (noDevice) pulled from inside an SDL3 audio callback.
//       Crackled/rasped badly - th07's BGM data source reads thbgm.dat off the
//       SD card on demand, so a blocking card read was happening on the
//       realtime audio thread, missing AUDOUT deadlines.
//
//   v2: same, but pushed from a feeder thread into an SDL3 stream. Better in
//       theory, still bad in practice: the chain was
//       engine -> SDL stream -> SDL switch backend -> AUDOUT, with SDL's own
//       double-buffered AUDOUT wrapper (2 buffers, no ownership tracking) at
//       the end of it.
//
//   v3 (this one): talk to libnx AUDOUT directly. No SDL audio subsystem at
//       all. Four page-aligned buffers are cycled strictly in the order AUDOUT
//       releases them, so a buffer is never refilled while it is still playing.
//       Everything runs at AUDOUT's native 48 kHz / stereo / S16, and the
//       miniaudio engine is configured to match, so no resampling happens
//       anywhere in the path.
//
// Card reads still happen on this thread, but they are absorbed by ~85 ms of
// queued buffers instead of stalling the hardware. Combined with preloadBgm
// (forced on for Switch in Supervisor::LoadConfig) the BGM is served from RAM.

#ifdef __SWITCH__

#include "SwitchAudio.hpp"

#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <switch.h>
#include <vector>

#include "miniaudio.h"

namespace
{

constexpr int kSampleRate = 48000;
constexpr int kChannels = 2;

// AUDOUT wants 0x1000-aligned memory and sizes.
// 1024 frames * 2ch * 2 bytes = 4096 bytes exactly -> ~21 ms per buffer.
constexpr int kFramesPerBuffer = 1024;
constexpr size_t kBufferBytes = (size_t)kFramesPerBuffer * kChannels * sizeof(s16);
constexpr int kNumBuffers = 4; // ~85 ms of slack for SD card hiccups
constexpr size_t kAlign = 0x1000;

AudioOutBuffer g_Buffers[kNumBuffers];
void *g_BufferMem[kNumBuffers];

Thread g_Thread;
bool g_ThreadStarted = false;
bool g_AudoutUp = false;
volatile bool g_Running = false;

ma_engine *g_Engine = nullptr;

// f32 scratch that the engine renders into before conversion to s16.
std::vector<float> g_Scratch;

void RenderInto(void *dst)
{
    ma_uint64 framesRead = 0;

    if (g_Engine == nullptr ||
        ma_engine_read_pcm_frames(g_Engine, g_Scratch.data(), (ma_uint64)kFramesPerBuffer,
                                  &framesRead) != MA_SUCCESS)
    {
        std::memset(dst, 0, kBufferBytes);
        return;
    }

    if ((int)framesRead < kFramesPerBuffer)
    {
        std::memset(g_Scratch.data() + framesRead * kChannels, 0,
                    ((size_t)kFramesPerBuffer - framesRead) * kChannels * sizeof(float));
    }

    ma_pcm_f32_to_s16(dst, g_Scratch.data(), (ma_uint64)kFramesPerBuffer * kChannels,
                      ma_dither_mode_none);
}

void AudioThread(void *)
{
    while (g_Running)
    {
        AudioOutBuffer *released = nullptr;
        u32 count = 0;

        // 100 ms timeout so shutdown cannot wedge on a silent device.
        if (R_FAILED(audoutWaitPlayFinish(&released, &count, 100000000ULL)))
        {
            continue;
        }
        if (released == nullptr || count == 0)
        {
            continue;
        }

        RenderInto(released->buffer);
        released->data_size = kBufferBytes;
        audoutAppendAudioOutBuffer(released);
    }
}

} // namespace

namespace SwitchAudio
{

bool Start(ma_engine *engine)
{
    g_Engine = engine;
    g_Scratch.assign((size_t)kFramesPerBuffer * kChannels, 0.0f);

    if (R_FAILED(audoutInitialize()))
    {
        std::printf("[th07-switch] audoutInitialize failed\n");
        return false;
    }
    g_AudoutUp = true;

    if (R_FAILED(audoutStartAudioOut()))
    {
        std::printf("[th07-switch] audoutStartAudioOut failed\n");
        audoutExit();
        g_AudoutUp = false;
        return false;
    }

    std::printf("[th07-switch] AUDOUT %lu Hz, %u ch\n", (unsigned long)audoutGetSampleRate(),
                (unsigned)audoutGetChannelCount());

    // Queue every buffer up front so playback starts with a full pipeline.
    for (int i = 0; i < kNumBuffers; i++)
    {
        g_BufferMem[i] = memalign(kAlign, kBufferBytes);
        if (!g_BufferMem[i])
        {
            std::printf("[th07-switch] out of memory for AUDOUT buffers\n");
            Stop();
            return false;
        }
        std::memset(g_BufferMem[i], 0, kBufferBytes);

        g_Buffers[i].next = nullptr;
        g_Buffers[i].buffer = g_BufferMem[i];
        g_Buffers[i].buffer_size = kBufferBytes;
        g_Buffers[i].data_size = kBufferBytes;
        g_Buffers[i].data_offset = 0;

        audoutAppendAudioOutBuffer(&g_Buffers[i]);
    }

    g_Running = true;

    // Priority 0x20 is a touch above the main thread (0x2C), pinned to core 1
    // so the renderer on core 0 cannot delay it. Core 2 is reserved by Horizon.
    if (R_FAILED(threadCreate(&g_Thread, AudioThread, nullptr, nullptr, 0x8000, 0x20, 1)) ||
        R_FAILED(threadStart(&g_Thread)))
    {
        std::printf("[th07-switch] audio thread failed to start\n");
        g_Running = false;
        Stop();
        return false;
    }
    g_ThreadStarted = true;

    std::printf("[th07-switch] audio up: direct AUDOUT, %d x %d frames (%d ms buffered)\n",
                kNumBuffers, kFramesPerBuffer, (kNumBuffers * kFramesPerBuffer * 1000) / kSampleRate);
    return true;
}

void Stop()
{
    if (g_ThreadStarted)
    {
        g_Running = false;
        threadWaitForExit(&g_Thread);
        threadClose(&g_Thread);
        g_ThreadStarted = false;
    }

    if (g_AudoutUp)
    {
        audoutStopAudioOut();
        audoutExit();
        g_AudoutUp = false;
    }

    for (int i = 0; i < kNumBuffers; i++)
    {
        if (g_BufferMem[i])
        {
            free(g_BufferMem[i]); // memalign() pairs with free()
            g_BufferMem[i] = nullptr;
        }
    }

    g_Engine = nullptr;
    g_Scratch.clear();
    g_Scratch.shrink_to_fit();
}

} // namespace SwitchAudio

#endif // __SWITCH__
