#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# th07-switch: one-shot build of the Nintendo Switch homebrew NRO.
#
#   ./scripts/build_switch.sh            # full build (deps + game)
#   ./scripts/build_switch.sh game       # rebuild only the game
#   ./scripts/build_switch.sh clean      # wipe build dirs
#
# Requirements on the host:
#   * devkitPro with devkitA64 + libnx + switch-portlibs (switch-mesa,
#     switch-glad/EGL, switch-freetype), i.e.
#       sudo dkp-pacman -S switch-dev switch-portlibs
#     or the official docker image devkitpro/devkita64.
#   * cmake >= 3.20, ninja, git, python3
#
# It builds, into ext/prefix, a Switch-targeted SDL3 stack:
#   SDL3 (with the libnx video/audio/joystick backend + our GLES3 context fix),
#   SDL3_image (stb JPEG only), SDL3_ttf (portlibs freetype),
# and then links th07 against it and wraps it into th07.nro.
#
# No game assets are downloaded or bundled. Bring your own th07.dat /
# thbgm.dat / msgothic.ttc from your legally owned copy of PCB 1.00b.
# ---------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXT="$ROOT/ext"
PREFIX="$EXT/prefix"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"
SDL_TAG="${SDL_TAG:-release-3.4.2}"
SDL_IMAGE_TAG="${SDL_IMAGE_TAG:-release-3.2.4}"
SDL_TTF_TAG="${SDL_TTF_TAG:-release-3.2.2}"

: "${DEVKITPRO:=/opt/devkitpro}"
export DEVKITPRO
TOOLCHAIN="$DEVKITPRO/cmake/Switch.cmake"

log() { printf '\033[1;35m[th07-switch]\033[0m %s\n' "$*"; }

case "${1:-all}" in
clean)
    rm -rf "$ROOT/build-switch" "$EXT/prefix" \
           "$EXT/SDL/build-switch" "$EXT/SDL_image/build-switch" "$EXT/SDL_ttf/build-switch"
    log "cleaned"
    exit 0
    ;;
esac

[ -f "$TOOLCHAIN" ] || { echo "devkitPro not found at $DEVKITPRO (set DEVKITPRO=...)"; exit 1; }
[ -d "$DEVKITPRO/portlibs/switch/include/GLES3" ] || {
    echo "switch-portlibs (mesa/EGL) missing: dkp-pacman -S switch-portlibs"; exit 1; }

build_deps() {
    mkdir -p "$EXT"

    # ---------------- SDL3 ----------------
    if [ ! -d "$EXT/SDL" ]; then
        log "cloning SDL3 $SDL_TAG"
        git clone --depth 1 -b "$SDL_TAG" https://github.com/libsdl-org/SDL.git "$EXT/SDL"
        log "applying libnx backend patch (neomody77/sdl3-switch) + GLES3 context fix"
        (cd "$EXT/SDL" \
            && git apply -p3 "$ROOT/platform/switch/sdl3-switch.patch" \
            && git apply "$ROOT/platform/switch/sdl3-switch-gles3.patch")
    fi
    log "building SDL3"
    cmake -S "$EXT/SDL" -B "$EXT/SDL/build-switch" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_BUILD_TYPE=Release \
        -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
        -DSDL_TEST_LIBRARY=OFF -DCMAKE_INSTALL_PREFIX="$PREFIX"
    cmake --build "$EXT/SDL/build-switch" -j"$JOBS"
    cmake --install "$EXT/SDL/build-switch"

    # ------------- SDL3_image -------------
    [ -d "$EXT/SDL_image" ] || git clone --depth 1 -b "$SDL_IMAGE_TAG" \
        https://github.com/libsdl-org/SDL_image.git "$EXT/SDL_image"
    log "building SDL3_image"
    cmake -S "$EXT/SDL_image" -B "$EXT/SDL_image/build-switch" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DBUILD_SHARED_LIBS=OFF -DSDLIMAGE_SAMPLES=OFF -DSDLIMAGE_TESTS=OFF \
        -DSDLIMAGE_DEPS_SHARED=OFF -DSDLIMAGE_BACKEND_STB=ON \
        -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF \
        -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_PNG=OFF -DSDLIMAGE_JPG=ON
    cmake --build "$EXT/SDL_image/build-switch" -j"$JOBS"
    cmake --install "$EXT/SDL_image/build-switch"

    # -------------- SDL3_ttf --------------
    [ -d "$EXT/SDL_ttf" ] || git clone --depth 1 -b "$SDL_TTF_TAG" \
        https://github.com/libsdl-org/SDL_ttf.git "$EXT/SDL_ttf"
    log "building SDL3_ttf"
    cmake -S "$EXT/SDL_ttf" -B "$EXT/SDL_ttf/build-switch" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DBUILD_SHARED_LIBS=OFF -DSDLTTF_SAMPLES=OFF -DSDLTTF_TESTS=OFF \
        -DSDLTTF_VENDORED=OFF -DSDLTTF_HARFBUZZ=OFF -DSDLTTF_PLUTOSVG=OFF
    cmake --build "$EXT/SDL_ttf/build-switch" -j"$JOBS"
    cmake --install "$EXT/SDL_ttf/build-switch"
}

build_game() {
    log "building th07.nro"
    cmake -S "$ROOT" -B "$ROOT/build-switch" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$PREFIX" ${TH_SWITCH_NXLINK:+-DTH_SWITCH_NXLINK=ON}
    cmake --build "$ROOT/build-switch" -j"$JOBS"
    cp "$ROOT/build-switch/th07.nro" "$ROOT/th07.nro"
    log "done -> $ROOT/th07.nro"
    ls -lh "$ROOT/th07.nro"
}

if [ "${1:-all}" != "game" ]; then
    build_deps
fi
build_game
