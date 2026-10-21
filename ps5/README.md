# YataiDON PS5 / prospero

The PS5 port lives on the `prospero` branch. Desktop/mobile backends remain in
place; the PS5 build selects its platform code with `YATAIDON_PLATFORM_PS5`.

## Current behavior

- Native folder app `PPSA99329`, launched through ShadowMount+.
- raylib / SDL2 / PS5 OpenGL 1.0.2, with a native 3840x2160 drawable.
- Don's 3D model uses 100% of that output pixel density.
- VSync is mandatory. Settings offers 60/120 buttons that select both frame
  pacing and output Hz; unsupported 120 Hz requests fall back to 60 Hz.
- Native 48 kHz stereo AudioOut with the game's mixer.
- Native filesystem wrappers, texture/audio/skin-JSON caches, and cancellable
  asynchronous song previews.
- Hardware H.264 Baseline/Main/High 8-bit 4:2:0 decoding through `sceVideodec2`.
  Other formats and decoder failures use FFmpeg software fallback. H.264 4:4:4
  clips need conversion to 4:2:0 to use the hardware path.
- Online services, skin downloads, MIDI and webcam are disabled on PS5.

Default DualSense bindings:

| Action | Buttons |
|---|---|
| Left Ka | D-pad Up/Left, L1, L2 |
| Left Don | D-pad Down/Right, L3 |
| Right Ka | Circle, Triangle, R1, R2 |
| Right Don | Square, Cross, R3 |
| Restart song | Options/Start |
| Back / leave song | Touchpad |

Menus use D-pad navigation, Cross to confirm, and Circle/touchpad to go back.
Fullscreen, borderless and VSync settings are hidden on PS5.

## Build in Linux / WSL

Requires the public PS5 Payload SDK with C++ support, CMake, Ninja, Git, curl,
and Python 3.12+. The local toolchain is `/opt/waddamburo-build/sdk` in the
`WaddamburoProspero` WSL distribution.

```sh
export PS5_PAYLOAD_SDK=/opt/waddamburo-build/sdk
bash ps5/prepare.sh
python3 ps5/prepare-game.py
export PS5_FFMPEG_PREFIX="$PWD/ps5/vendor/ffmpeg-ps5"
bash ps5/build-game.sh
python3 ps5/package.py --skip-media
python3 ps5/tools/verify-bundle.py
```

The SDK and dependency inputs have been copied into the ignored `ps5/vendor`
folder in this local checkout. Generated CMake builds are recreated here, not
copied from the previous checkout. The game builder bootstraps SDL and installs
a matching 4K display profile before building the game.

For a fresh clone, `prepare.sh` fetches pinned OpenGL/raylib/SDL inputs and
`prepare-game.py` fetches the game libraries. Build a PS5 FFmpeg prefix with
`ps5/build-ffmpeg.sh` before the game build: set `FFMPEG_SOURCE` to FFmpeg 8.1.2
sources and `PS5_OPUS_PREFIX` to a PS5 libopus prefix. The script enables MP4/MOV,
H.264, AAC and the supported audio codecs. `FFMPEG_BUILD_DIR` can point to the
Linux filesystem for faster compilation. Packaging rejects incomplete prefixes.

The native packaging template supplies public tooling, a prepared host zlib,
and its checksum-verified clean-room `runtime/libc.prx`. The local checkout has
these inputs under ignored `ps5/vendor/native-template`, the packager default,
so it no longer needs the previous checkout or the sibling Waddamburo checkout
to build. For a fresh clone, supply a compatible public native-app template
with `ps5/package.py --template /path/to/template`, or prepare that default
directory. The template is a packaging input, not the renderer.

## Install

Replace the complete `Homebrew/PPSA99329` app folder, including `sce_sys`, and
remount/register it through ShadowMount+. The icon is `ps5/assets/icon0.png`.

Game data lives at `/data/YataiDON` on the console. Install complete skins and
parent dependencies under `/data/YataiDON/Skins`, and songs under
`/data/YataiDON/Songs`. Git LFS pointers are not usable skin assets. Configuration,
scores and `native.log` are stored in that data folder.

`ps5/dist/PPSA99329` is the rebuilt native app. Local migration also includes
`YataiDON-PS5-hardware-video-v4.zip` and `YataiDON-PS5-hardware-compatible-title.zip`
in `ps5/dist`; the latter contains a converted Nijiiro title clip for `/data`.
`--skip-media` packages the app and shaders while leaving installed skins and
songs out of the ZIP; omit it to build a full data bundle.

Generated packages, dependencies, console logs/config and downloaded skins are
ignored by Git. Do not distribute the intermediate payload link-check executable.

## Verification and known limits

On-console logs confirmed native 4K/120 Hz output and successful hardware H.264
frames for `2011_OP.mp4` (1280x720). Model rendering and controller operation
have also been tested by the user. Hardware decode uses the native title import
mechanism; linker-only decoder stub bodies are not packaged or executed.

Video still converts NV12 to RGBA on the CPU and uploads textures. Console
measurements were approximately 13 ms per 720p upload and 30 ms per 1080p upload;
hardware decompression alone therefore does not guarantee 120 FPS playback.
The remaining looping/SFX reports need further console verification. The later
report of a silent song track was withdrawn by the user as working.

Host regression tests cover mixer looping/one-shots/retrigger serialization,
bounded preview-worker scheduling, asset-cache ownership/eviction, H.264 Annex-B
conversion, native audio output, filesystem/directory/memory adapters, and thread
cleanup. Run `python3 ps5/test-mixer.py`; the other `ps5/*-test.cpp` and
`ps5/*-test.c` files are host test programs. `ps5/tools/verify-bundle.py` checks
native file hashes, title/HFR metadata, SELF magic and package integrity.

## Sources and licensing

YataiDON and the PS5 OpenGL integration are GPL-3.0. OpenGL/raylib/SDL and game
library revisions are pinned in the preparation scripts. Dependency licenses
are included by the packager.

The public hardware decoder ABI/call sequence comes from
[EVO Player](https://github.com/sainsaji/EVO-PLAYER-PS5), commit
`875f766dc9de762c1a1f608005d0afd1f25116f4`, based on ProsperoLight. Its provenance
is retained in `src/platform/ps5_videodec2_abi.h` and `ps5/videodec-link-stub.c`.
No Sony SDK headers, firmware modules or proprietary decoder binaries are bundled.
