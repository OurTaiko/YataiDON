# YataiDON for PS5 / prospero

Experimental native folder app `PPSA99329`. This branch follows upstream's
backend-neutral graphics API and uses **bgfx + Vulkan** on PS5. Desktop/mobile
configuration remains on upstream's normal path.

## Status

The Vulkan backend replaces the old raylib/OpenGL console renderer. SDL2 now
provides the logical window and DualSense input; it does not create a GL context
or present frames. bgfx uses the linked PS5 Vulkan RADV driver and a native
`VK_KHR_display` surface. SPIR-V shaders are compiled by a matching host shaderc.

The bgfx backend includes upstream's glTF model loading, skeletal animation,
mesh rendering and outline passes. PS5 compiles these shaders to SPIR-V for
Vulkan. Don's model uses 100% of the active output's pixel density, including
2160p when the selected display mode supports it. The new 3D path requires
on-console rendering and performance validation.

The display bridge selects the largest advertised mode up to 3840x2160. VSync
is mandatory. The console settings offer 60/120 Hz, with HDMI capability checking
and fallback to 60 Hz. The new Vulkan renderer and mode switching still require
console validation; the previous OpenGL build's hardware results do not validate
this replacement.

Retained console features:

- Native 48 kHz stereo AudioOut and upstream loudness normalization.
- Native filesystem compatibility and bounded texture/audio/skin JSON caches.
- Cancellable asynchronous song previews.
- Native PS5 on-screen keyboard for song search in both 1P and 2P and settings
  text fields. Existing settings text is prefilled for editing; confirmation
  replaces the field and cancellation keeps the saved value. Unicode text is
  supported, with gamepad input blocked while the dialog is open.
- Native H.264 Baseline/Main/High 8-bit 4:2:0 video decoding with software fallback.
- User data and logs under `/data/YataiDON` (`native.log`).
- RADV shader cache under `/data/YataiDON/radv-shader-cache` (including Mesa's cache-format subdirectories).
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

Both players use this layout. In two-player songs, the first connected pad
controls P1 and the second controls P2. Disconnecting a pad keeps the other
player's assignment. Player-specific entry, costume and difficulty menus use
the corresponding pad, including P2's join request. Shared menus (such as the
title screen) and single-player songs accept either pad.
Older empty P2 layouts are filled automatically once; custom P2 bindings are
preserved.

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

Run `python3 ps5/tools/verify-bundle.py` after packaging to check executable
hashes, metadata and archive integrity. These checks do not prove rendering,
HDMI output, audio timing or console stability.

Before treating a build as a release, check onboarding, entry, song selection,
settings, song playback, Options restart/touchpad back, video playback, 60/120 Hz
switching and shutdown on a console. Keep the last working build for comparison.

## Acknowledgements and source references

Thank you to the authors and contributors of the projects below. Their public
code, tooling and documentation made this port possible. The descriptions
identify what YataiDON uses directly, what comes through its graphics driver,
and what was consulted during development.

| Repository | Contribution to this port |
| --- | --- |
| [YataiDON — Yonokid and contributors](https://github.com/Yonokid/YataiDON) | The game and shared graphics API, including the bgfx renderer, glTF models, skeletal animation and outline shaders merged into this fork. |
| [PS5 Payload SDK — ps5-payload-dev / John Törnblom](https://github.com/ps5-payload-dev/sdk) | Public PS5 headers, import stubs, sysroot and Prospero compiler/linker toolchain used to cross-compile the game and its dependencies. |
| [SDL for PS5 — ps5-payload-dev](https://github.com/ps5-payload-dev/SDL) | SDL2 controller support and the public PS5 keyboard backend from which `native-ime.c`'s IME ABI and lifecycle were adapted. The port supplies its own logical-window backend for Vulkan. |
| [PS5_Vulkan — Mihawk-99](https://github.com/mihawk-99/PS5_Vulkan) | The linked Vulkan driver build, RADV integration and PS5 display/presentation support. Also supplies the AGC import descriptions used when linking the native app. |
| [PS5_Mesa — Mihawk-99 and Mesa contributors](https://github.com/mihawk-99/PS5_Mesa) | The Mesa fork underlying the selected RADV driver, including its PS5 winsys and VideoOut WSI. RADV, NIR and ACO provide the Vulkan implementation and GPU shader compilation. |
| [PS5_PayloadSDK — Mihawk-99](https://github.com/mihawk-99/PS5_PayloadSDK) | SDK/platform-layer work used by the PS5 Vulkan dependency stack, including native memory and platform compatibility support. |
| [PS5 Native App Boilerplate — BlackBearReloaded](https://github.com/blackbearreloaded/ps5-native-app-boilerplate) | Native startup, ELF/module conversion and FSELF packaging tools, plus the generated clean-room `libc.prx` compatibility shim supplied through the local native-app template. |
| [EVO Player — sainsaji and contributors](https://github.com/sainsaji/EVO-PLAYER-PS5) | Hardware-video decoder ABI declarations and initialization/call-sequence reference for `platform_ps5_video.cpp` and `ps5_videodec2_abi.h`, based on commit `875f766dc9de762c1a1f608005d0afd1f25116f4`. |
| [ProsperoLight — BlackBearReloaded](https://github.com/blackbearreloaded/ProsperoLight) | Original VideoDec2 structure layouts and native decoding sequence behind the EVO Player reference used here. |
| [ShadowMountPlus — drakmor and contributors](https://github.com/drakmor/ShadowMountPlus) | The folder-app registration and launch environment used during console testing; informed the title metadata and required launcher-asset layout. It is installed separately. |

### Earlier renderer and additional references

| Repository | How it was used or referenced |
| --- | --- |
| [PS5 OpenGL — BlackBearReloaded](https://github.com/blackbearreloaded/ps5-opengl), including the [darkxex mirror](https://github.com/darkxex/ps5-opengl) supplied for this port | Powered the original raylib/OpenGL PS5 build before the bgfx/Vulkan migration. Native runtime/logging helpers, linker script and symbol-map material retain their source notices. The current renderer uses Vulkan. |
| [opengnm-psbc — PS4-OpenGNM](https://github.com/PS4-OpenGNM/opengnm-psbc) | Mesa-derived shader-compiler work credited by the PS5 OpenGL/Vulkan projects; part of the earlier graphics stack and driver research lineage, rather than a separate game dependency. |
| [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) | Public IME dialog declarations consulted alongside the PS5 SDL backend when checking keyboard parameter layouts and alignment enums. A reference, rather than the toolchain used to build this PS5 app. |
| [shadPS4](https://github.com/shadps4-emu/shadPS4) | Public IME implementation consulted for coordinate ranges, alignment and fixed-position/automatic-adjustment options when centering the native keyboard. Reference only. |
| [bgfx](https://github.com/bkaradzic/bgfx) and [bgfx.cmake](https://github.com/bkaradzic/bgfx.cmake) | Shared renderer, SPIR-V shader tooling and CMake integration, with a small PS5 platform patch maintained in `ps5/patches/`. |
| [raylib](https://github.com/raysan5/raylib) | Renderer used by the earlier PS5 build; its external headers remain a source for image/font decoding and glTF parsing in the bgfx build. |

Other shared game dependencies are credited in the [main README](../README.md).
Source-file copyright and licence notices remain in place; this acknowledgement
supplements them. BlackBearReloaded's runtime/tooling and Mihawk-99's AGC import
descriptions retain their GPL-3.0-or-later notices, and the SDL-derived IME
retains its zlib notice.
Redistributed driver notices are kept under `ps5/runtime/licenses` and included
by the packaging script.

## Dependency revisions

- Upstream merge: `b88825f` (2026-10-27 snapshot).
- PS5 SDL: `8c56053f13ca13a0c050de613706ff69eb615836`.
- bgfx.cmake `v1.167.9590-598`, matching upstream. The release tarball SHA256 and
  original/patched file hashes are checked by preparation tools.
- The driver used for local build validation is PS5 Vulkan `f410e67a`, with Mesa
  `1ba9874f` and release archive SHA256
  `85d0ac684b3b74e446992150a7fbb04f543a78429a63e0b25a0cbf1cbac26dfc`.
- Generated builds, libraries, media and logs are ignored; they are not PR inputs.
