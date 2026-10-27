# YataiDON for PS5 / prospero

Experimental native folder app `PPSA99329`. This branch follows upstream's
backend-neutral graphics API and uses **bgfx + Vulkan** on PS5. Desktop/mobile
configuration remains on upstream's normal path.

## Status

The Vulkan backend replaces the old raylib/OpenGL console renderer. SDL2 now
provides the logical window and DualSense input; it does not create a GL context
or present frames. bgfx uses the linked PS5 Vulkan RADV driver and a native
`VK_KHR_display` surface. SPIR-V shaders are compiled by a matching host shaderc.

**Upstream bgfx does not implement 3D models yet.** Its model entry points are
placeholders, so Don's 3D model is absent with this backend. The retained PS5
model scale is 100% of the drawable's pixel density when model support becomes
available. Implementing bgfx model rendering is a separate, shared-backend task.

The display bridge selects the largest advertised mode up to 3840x2160. VSync
is mandatory. The console settings offer 60/120 Hz, with HDMI capability checking
and fallback to 60 Hz. The new Vulkan renderer and mode switching still require
console validation; the previous OpenGL build's hardware results do not validate
this replacement.

Retained console features:

- Native 48 kHz stereo AudioOut and upstream loudness normalization.
- Native filesystem compatibility and bounded texture/audio/skin JSON caches.
- Cancellable asynchronous song previews.
- Native H.264 Baseline/Main/High 8-bit 4:2:0 video decoding with software fallback.
- User data and logs under `/data/YataiDON` (`native.log`).
- Online services, skin downloads, MIDI and webcam disabled on PS5.

## Controls

| Action | DualSense buttons |
|---|---|
| Left Ka | D-pad Up/Left, L1, L2 |
| Left Don | D-pad Down/Right, L3 |
| Right Ka | Circle, Triangle, R1, R2 |
| Right Don | Square, Cross, R3 |
| Restart song | Options/Start |
| Back / leave song | Touchpad |

Menus use D-pad navigation, Cross to confirm, Circle/touchpad to go back.
Fullscreen, borderless and VSync controls are hidden on the console.

## Build

Use Linux or WSL with CMake 3.24+, Ninja, Python 3.11+, Git, Clang and a
host C++ compiler. Install the public PS5 payload SDK and build the release RADV
archive using [PS5 Vulkan](https://github.com/mihawk-99/PS5_Vulkan).
Build FFmpeg using `ps5/build-ffmpeg.sh`; its installation must include MOV/MP4,
H.264, AAC and Opus support. No Sony SDK or proprietary shader compiler is used.

```sh
export PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk
export PS5_FFMPEG_PREFIX=/path/to/ffmpeg-ps5
export PS5_VULKAN_PREFIX=/path/to/PS5_Vulkan/.deps/native/radv-release
BUILD_JOBS=8 bash ps5/build-game.sh
python3 ps5/package.py --skip-media
python3 ps5/tools/verify-bundle.py
```

`build-game.sh` fetches pinned source dependencies, applies a checksum-verified
bgfx patch, builds shaderc for the host, then cross-compiles into
`ps5/build-vulkan`. Set `YATAIDON_HOST_BGFX_BUILD` to reuse a host shaderc build.
The main CMake entry point also selects `ps5/` when the SDK toolchain sets `PS5`.

Packaging expects a public native-app template in `ps5/vendor/native-template`
(or `--template /path/to/template`), including `tooling/native`, the clean-room
`runtime/libc.prx` plus its checksum, and host zlib under
`.deps/native/zlib/root/usr`. Generated import stubs describe AGC/Videodec2
firmware imports; their placeholder bodies never ship in the executable.

Output: `ps5/dist/YataiDON-PS5-experimental.zip`. Replace the complete
`PPSA99329` app folder using your folder-app installation workflow. With
`--skip-media`, existing skins, songs, settings and saves remain external;
install complete skins and songs under `/data/YataiDON`. The build receipt
records the executable, game archive and linked driver hashes.

## Verification

Host regression tests cover display discovery, mode selection and missing
Vulkan entry points, refresh rejection/restoration, the actual audio mixer's
looping/retrigger behavior, bounded preview work and H.264 packet conversion.
They do not prove GPU rendering, HDMI output, audio timing or console stability.

```sh
python3 ps5/test-mixer.py
cc -std=c11 -fsanitize=address,undefined -I"$PS5_VULKAN_PREFIX/include" \
  ps5/vulkan-display-test.c -o /tmp/yataidon-vulkan-display-test
/tmp/yataidon-vulkan-display-test
cc ps5/display-test.c -o /tmp/yataidon-display-test
/tmp/yataidon-display-test
```

Before treating a build as a release, check onboarding, entry, song selection,
settings, song playback, Options restart/touchpad back, video playback, 60/120 Hz
switching and shutdown on a console. Keep the last working build for comparison.

## Dependency notes and attribution

- Upstream merge: `c59f6ac` (2026-10-27 snapshot).
- bgfx.cmake `v1.167.9590-598`, matching upstream. The release tarball SHA256 and
  original/patched file hashes are checked by preparation tools.
- The native display/procedure bridge and bgfx adaptation follow Waddamburo's
  MIT-licensed native PS5 bridge; the patch is kept outside vendored sources.
- The driver used for local build validation is PS5 Vulkan `f410e67a`, with Mesa
  `1ba9874f` and release archive SHA256
  `85d0ac684b3b74e446992150a7fbb04f543a78429a63e0b25a0cbf1cbac26dfc`.
- Runtime linker/logging helpers retain BlackBearReloaded's GPL-3.0-or-later
  notices. AGC import descriptions retain Mihawk-99's GPL notices.
- Generated builds, libraries, media and logs are ignored; they are not PR inputs.
