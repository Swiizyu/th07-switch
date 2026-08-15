# Touhou 7: Perfect Cherry Blossom — Nintendo Switch Port
*(東方妖々夢　〜 Perfect Cherry Blossom)*

![Platform](https://img.shields.io/badge/Platform-Nintendo%20Switch-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![Status](https://img.shields.io/badge/Status-Fully%20Playable-brightgreen?style=for-the-badge)
![License](https://img.shields.io/badge/License-CC0%201.0-blue?style=for-the-badge)

A native homebrew port of ZUN's 2003 bullet hell danmaku classic **Touhou 7: Perfect Cherry Blossom** for the **Nintendo Switch** (Horizon OS).

Built on the [some100/th07](https://github.com/some100/th07) decompilation (`reallyportable` branch), this port runs the game through SDL3 on an OpenGL ES 3 context over Nouveau/Mesa, talks to the console's audio hardware directly via libnx AUDOUT, and paces itself to a locked 60 FPS on Horizon — no Linux, Box64 or Wine involved.

Companion to the [Touhou 6 Switch port](https://github.com/Swiizyu/th06-switch), with matching controls.

---

## ✨ Key Features

* 🚀 **Locked 60 FPS:** Horizon's EGL implementation does not block on swap, so the renderer would otherwise free-run at ~120 FPS while the logic ticked at 60. The port paces presentation against an absolute deadline, giving stable frames and noticeably less battery drain.
* 🌸 **OLED-Friendly Pillarboxing:** The original 640×480 playfield is centred inside the Switch's 1280×720 display with pure black (`#000000`) bars.
* 🔊 **Direct AUDOUT Audio:** Rather than layering the miniaudio engine on top of SDL's audio backend, the port drives libnx AUDOUT itself with four page-aligned buffers cycled in strict release order at the hardware's native 48 kHz — no resampling anywhere in the path. BGM is preloaded into RAM so SD card reads never stall playback.
* 🎮 **Fixed, Sane Controls:** Joy-Con (handheld, grip, detached) and Pro Controller via SDL3's gamepad API.
* 🌏 **Language-Aware Title:** hbmenu shows the original Japanese title on consoles set to 日本語 and the romanised one everywhere else, filled across all 16 NACP language slots.
* 📁 **Flexible Data Location:** The game data can sit in `sd:/switch/th07/`, `sd:/th07/`, `sd:/touhou7/`, `sd:/games/th07/` and more — folder names are matched case-insensitively against real directory entries, so FAT32 capitalisation quirks do not matter.
* 💾 **Saves Next to the Data:** `th07.cfg`, `score.dat`, replays and snapshots are written into the same SD folder the game loaded from.

---

## 📥 Installation Guide

> ⚠️ **Disclaimer:** In compliance with ZUN's guidelines and copyright law, this repository contains **ONLY the homebrew engine code**. No game assets are distributed. You must legally own a copy of *Touhou 7: Perfect Cherry Blossom v1.00b*.

### 1. SD Card File Structure

1. Ensure your Nintendo Switch is running custom firmware (Atmosphère CFW).
2. Download the latest `touhou07.nro` from the [Releases](../../releases) tab (or build from source).
3. Create a folder named `sd:/switch/th07/` and copy the following into it:

```text
sd:/switch/th07/
    ├── touhou07.nro          # Nintendo Switch homebrew executable
    ├── th07.dat          # Main game archive
    ├── thbgm.dat         # Background music archive
    └── msgothic.ttc      # Japanese font (ships with the Windows release)
```

The loader also accepts `sd:/th07/`, `sd:/touhou7/`, `sd:/touhou 7/`, `sd:/switch/touhou7/`, `sd:/games/th07/`, `sd:/roms/th07/` and `pcb` / `perfect cherry blossom` variants, in any capitalisation — or simply the folder the NRO was launched from.

### 2. Music

Unlike Touhou 6, no extra soundtrack download is needed: PCB ships its BGM inside `thbgm.dat`, and the port plays it directly. MIDI mode is unavailable (Horizon has no system synthesizer) and is not needed.

### 3. Launching

Run `touhou07.nro` from the **Homebrew Menu (hbmenu)**, **Sphaira launcher**, or a home screen forwarder.

---

## 🕹 Controls

| Nintendo Switch Button | Action |
| :--- | :--- |
| **Left Stick / D-Pad** | Character Movement |
| **A** | Bomb / Cancel |
| **B** | Shoot / Confirm |
| **L** | Focus (Precision Slow-Motion Movement) |
| **R** | Skip Dialogue (hold) |
| **+ (Plus)** | Pause / In-Game Menu |


Every other button is intentionally inert. The layout is fixed in code rather than read from `th07.cfg`, because the config format cannot express the triggers and its PC defaults bind a debug cheat key to **+**.

ZUN's "shot slow" auto-focus (holding shot also engages focus) defaults to **off** here, but can be turned back on in **Option** and will be respected.

---

## 🛠 Building from Source

### Automated Build (GitHub Actions)

This repository includes a CI pipeline (`.github/workflows/build-switch.yml`). Push or fork the repository and the workflow compiles `th07.nro` inside the official `devkitpro/devkita64` container, uploading it as a downloadable artifact.

### Local Build (Linux / macOS / WSL)

1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with `devkitA64` and `libnx`.
2. Install the required Switch portlibs:

   ```bash
   sudo dkp-pacman -Syu
   sudo dkp-pacman -S switch-dev switch-mesa switch-libdrm_nouveau switch-freetype switch-libpng switch-libjpeg-turbo switch-zlib
   ```

3. Build:

   ```bash
   export DEVKITPRO=/opt/devkitpro
   ./scripts/build_switch.sh          # SDL3 stack + game
   ./scripts/build_switch.sh game     # rebuild the game only
   ```

   The result is `touhou07.nro` in the repository root.

The script cross-compiles an SDL3 stack into `ext/prefix` first, because devkitPro only ships SDL2 for Switch and the official SDL3 Switch port is NDA-gated:

* **SDL3** `release-3.4.2` + `platform/switch/sdl3-switch.patch` — the libnx video/audio/joystick backend from [neomody77/sdl3-switch](https://github.com/neomody77/sdl3-switch) (zlib) — plus `platform/switch/sdl3-switch-gles3.patch`, which makes the EGL context honour `SDL_GL_SetAttribute` so the game gets GLES 3 instead of GLES 2. Without it the VAO-based renderer will not run.
* **SDL3_image** (stb JPEG backend only) and **SDL3_ttf** (portlibs freetype, no harfbuzz).

Debugging on hardware:

```bash
TH_SWITCH_NXLINK=1 ./scripts/build_switch.sh game
nxlink -s th07.nro
```

---

## 📂 What This Fork Changes

Everything Switch-specific is confined to `platform/switch/` or guarded by `#ifdef __SWITCH__`, keeping rebases onto upstream cheap:

| File | Purpose |
| :--- | :--- |
| `platform/switch/PlatformSwitch.cpp` | romfs, optional nxlink stdio, SD data folder discovery |
| `platform/switch/SwitchAudio.cpp` | miniaudio → libnx AUDOUT bridge |
| `platform/switch/icon.jpg` | 256×256 NRO icon |
| `scripts/build_switch.sh` | one-shot dependency + game build |
| `scripts/nacp_lang.py` | per-language NACP titles |
| `src/GameWindow.cpp` | 60 FPS frame pacer |
| `src/Controller.cpp` | fixed Switch button layout |
| `src/SoundPlayer.cpp` | device-less miniaudio engine at 48 kHz |
| `src/FileSystem.cpp`, `src/Supervisor.cpp`, `src/main.cpp` | SD paths, config defaults, startup hooks |

One upstream bug was fixed along the way: `ResultScreen::OpenScore` allocated `ScoreDat` with a bare `new`, leaving `decodedData` uninitialised. On desktop, fresh OS pages happen to be zero and the "no score.dat yet" path survives; on Horizon the heap is dirty, so `ReleaseScoreDat` called `free()` on garbage and the game crashed on first launch. Worth upstreaming.

---

## 🤝 Credits & Acknowledgments

* **ZUN / Team Shanghai Alice** — original creator and developer of the Touhou Project series.
* **[some100](https://github.com/some100/th07)** — the Touhou 7 decompilation and its cross-platform branches.
* **[neomody77](https://github.com/neomody77/sdl3-switch)** — SDL3 libnx backend.
* **Switchbrew & devkitPro Team** — the open-source `libnx` SDK and Switch toolchain.

