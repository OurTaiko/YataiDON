"""Assemble a native title from the game archives and a local public native-app template."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent


def run(*args, **kwargs):
    subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--template", type=Path, default=ROOT / "vendor/native-template")
    parser.add_argument("--tools-only", action="store_true")
    parser.add_argument("--skip-media", action="store_true", help="Package the app and shaders without installed skins/songs")
    args = parser.parse_args()
    template = args.template.resolve()
    sdk = Path(os.environ["PS5_PAYLOAD_SDK"]).resolve()
    ffmpeg = Path(os.environ["PS5_FFMPEG_PREFIX"]).resolve()
    prefix = Path(os.environ.get("PS5_OPENGL_PREFIX", ROOT / "vendor/opengl-sdk/ps5-opengl-sdk-1.0.2/sdk")).resolve()
    graphics = ROOT / "vendor/ps5-opengl"
    build = ROOT / "build-game"
    stage = ROOT / "package"
    app = ROOT / "dist/PPSA99329"
    for directory in (stage / "tooling/native", stage / "objects", app / "sce_sys", app / "sce_module"):
        directory.mkdir(parents=True, exist_ok=True)
    upstream_native = template / "tooling/native"
    native = stage / "tooling/native"
    for source in upstream_native.iterdir():
        if source.is_file() and source.suffix in (".cpp", ".hpp", ".h"):
            shutil.copy2(source, native / source.name)
    writer = native / "sce_module_writer.cpp"
    writer_text = writer.read_text()
    old_heap = "write_u64(result.data, result.heap_size, std::numeric_limits<std::uint64_t>::max());"
    if old_heap not in writer_text:
        raise RuntimeError("Unexpected native template heap declaration")
    writer.write_text(writer_text.replace(old_heap, "write_u64(result.data, result.heap_size, 0x10000000ULL);"))
    runtime = template / "runtime/libc.prx"
    run("sha256sum", "--check", "--strict", "libc.prx.sha256", cwd=template / "runtime")
    # Preserve upstream linker-layout license and provide the graphics EH bounds.
    shutil.copy2(upstream_native / "ps5-pie.ld", stage / "tooling/native/ps5-pie-base.ld")
    for name in ("ps5-pie.ld", "app-symbols.map"):
        shutil.copy2(graphics / "native-app" / name, stage / "tooling/native" / name)
    host_tool = stage / "ps5-native-tool"
    zlib = template / ".deps/native/zlib/root/usr"
    tool_stamp = stage / "tool-inputs.sha256"
    tool_identity = hashlib.sha256("".join(digest(p) for p in sorted(native.iterdir())
                                         if p.suffix in (".cpp", ".hpp", ".h")).encode()).hexdigest()
    if not host_tool.exists() or not tool_stamp.exists() or tool_stamp.read_text() != tool_identity:
        run("c++", "-std=c++20", "-O2", "-I", zlib / "include",
            *[native / name for name in ("native_app_builder.cpp", "self_container.cpp", "elf_object.cpp", "sce_module_writer.cpp")],
            zlib / "lib/libz.a", "-o", host_tool)
        tool_stamp.write_text(tool_identity)
    if args.tools_only:
        return
    sources = [(native / "app_crt.cpp", "crt.o"),
               (graphics / "native-app/app_heap.c", "heap.o"),
               (ROOT / "native-thread-cleanup.c", "thread-cleanup.o"),
               (ROOT / "native-filesystem.c", "filesystem.o"),
               (ROOT / "native-memory.c", "memory.o"),
               (ROOT / "native-directory.c", "directory.o")]
    sources.append((ROOT / "native-display.c", "display.o"))
    # Keep logs in the writable data folder, without the diagnostic's return hold.
    shims = (graphics / "native-app/runtime_shims.c").read_text()
    start = shims.index("__attribute__((noreturn)) void catchReturnFromMain")
    end = shims.index("__attribute__((noreturn)) void __assert", start)
    shims = "#include <sys/stat.h>\n" + shims[:start] + shims[end:]
    shims = shims.replace("static void ps5_opengl_open_log(void) {",
                          "static void ps5_opengl_open_log(void) {\n  (void)mkdir(\"/data/YataiDON\", 0777);")
    shims = shims.replace("setvbuf(stream, NULL, _IONBF, 0);\n}",
                          "setvbuf(stream, NULL, _IONBF, 0);\n  fprintf(stderr, \"[yataidon-ps5] hardware-video-v4\\n\");\n}")
    shims = shims.replace("setvbuf(stream, NULL, _IONBF, 0);", "setvbuf(stream, NULL, _IOLBF, 65536);")
    # Prefer the complete payload SDK implementations when its libc archive
    # also supplies these diagnostic fallback functions.
    for declaration in ("int mkstemps(", "void openlog(", "FILE *popen(", "int pclose("):
        shims = shims.replace(declaration, "__attribute__((weak)) " + declaration)
    (stage / "runtime.c").write_text(shims.replace("/download0/ps5-opengl.log", "/data/YataiDON/native.log"))
    (stage / "budget.c").write_text("#include <stddef.h>\nconst size_t ps5_opengl_heap_size = 512u * 1024u * 1024u;\n")
    sources.extend([(stage / "runtime.c", "runtime.o"), (stage / "budget.c", "budget.o")])
    objects = []
    for source, name in sources:
        output = stage / "objects" / name
        compiler = sdk / ("bin/prospero-clang++" if source.suffix == ".cpp" else "bin/prospero-clang")
        run(compiler, "-O2", "-fPIC", "-ffunction-sections", "-fdata-sections", "-c", source, "-o", output)
        objects.append(output)
    archives = [Path(line) for line in (build / "native-archives.txt").read_text().splitlines()]
    archives.extend(ffmpeg / f"lib/lib{name}.a" for name in ("avformat", "avcodec", "swscale", "swresample", "avutil", "opus"))
    archives.extend(sdk / f"target/lib/{name}.a" for name in ("libc++", "libc++abi", "libc++experimental", "libunwind", "libc", "libpthread", "libdl", "libps5platform"))
    archives.append(prefix / "lib/libPS5OpenGL.a")
    resource = subprocess.check_output([str(sdk / "bin/prospero-clang"), "--print-resource-dir"], text=True).strip()
    archives.append(Path(resource) / "lib/linux/libclang_rt.builtins-x86_64.a")
    for path in archives:
        if not path.is_file():
            raise FileNotFoundError(path)
    for library, required in (("avformat", (b"ff_mov_demuxer\0",)),
                              ("avcodec", (b"ff_h264_decoder\0", b"ff_aac_decoder\0"))):
        symbols = (ffmpeg / f"lib/lib{library}.a").read_bytes()
        if any(symbol not in symbols for symbol in required):
            raise RuntimeError("FFmpeg needs MP4/MOV, H.264 and AAC support; run ps5/build-ffmpeg.sh")
    intermediate = stage / "game.elf"
    gl_stubs = [prefix / f"lib/{name}.so" for name in ("libSceAgc", "libSceAgcDriver")]
    gl_stubs.append(build / "libSceVideodec2.so")
    run(sdk / "bin/prospero-lld", "-T", "tooling/native/ps5-pie.ld", "--eh-frame-hdr",
        "--no-dynamic-linker",
        "--version-script", "tooling/native/app-symbols.map", "--exclude-libs=ALL",
        *[f"--wrap={name}" for name in ("malloc", "calloc", "realloc", "free", "posix_memalign", "malloc_usable_size", "stat", "lstat", "fstat",
                                       "fopen", "freopen", "open", "opendir", "mkdir", "unlink", "rmdir", "remove", "rename", "access", "realpath", "getcwd", "chdir",
                                       "mmap", "munmap", "mprotect", "pathconf", "readdir", "closedir", "dirfd", "fdopendir",
                                       "sceVideoOutOpen", "sceVideoOutClose")],
        "-L", prefix / "lib", "-L", sdk / "target/lib", "-u", "ps5_agc_gate2_run",
        "-e", "_start", "-o", intermediate, *objects,
        "--start-group", *archives, "--end-group", "--as-needed", *gl_stubs,
        *sorted((sdk / "target/lib").glob("*.so")), cwd=stage)
    elf = stage / "eboot.elf"
    run(host_tool, "link", "--in", intermediate, "--out", elf,
        "--stub-dir", sdk / "target/lib", "--stub", gl_stubs[0], "--stub", gl_stubs[1], "--stub", gl_stubs[2],
        "--module-sdk", "0x02000009", "--companion-sdk", "0x08050001", "--file-name", "eboot.elf")
    run(host_tool, "self", "--sign", "--in", elf, "--out", app / "eboot.bin", "--magic", "0x1D3D154F")
    shutil.copy2(runtime, app / "sce_module/libc.prx")
    param = json.loads((graphics / "native-app/param.json").read_text())
    param.update(titleId="PPSA99329", conceptId="99329", contentId="UP9000-PPSA99329_00-YATAIDON00000001", contentVersion="01.000.005")
    # The shell rejects high-refresh ConfigureOutput requests without both
    # capability bits, even when the HDMI sink reports 120 Hz support.
    param["attribute3"] = int(param.get("attribute3", 0)) | 0x80040
    param["localizedParameters"]["en-US"]["titleName"] = "YataiDON"
    (app / "sce_sys/param.json").write_text(json.dumps(param, indent=2) + "\n")
    # ShadowMount+ aborts title staging if this mandatory icon is absent.
    icon = ROOT / "assets/icon0.png"
    icon_bytes = icon.read_bytes()
    if len(icon_bytes) < 24 or icon_bytes[:8] != b"\x89PNG\r\n\x1a\n" or struct.unpack(">II", icon_bytes[16:24]) != (512, 512):
        raise RuntimeError("The native title icon must be a 512x512 PNG")
    shutil.copy2(icon, app / "sce_sys/icon0.png")
    data = ROOT / "dist/data/YataiDON"
    data.mkdir(parents=True, exist_ok=True)
    for name in ("shader", "Songs", "Skins"):
        if args.skip_media and name != "shader":
            continue
        source = ROOT.parent / name
        if source.is_dir():
            shutil.copytree(source, data / name, dirs_exist_ok=True, ignore=shutil.ignore_patterns(".git"))
    (ROOT / "dist/INSTALL.txt").write_text(
        "YataiDON PS5 - experimental, not hardware tested\n\n"
        "Install/register PPSA99329 using your native homebrew folder-app workflow.\n"
        "Copy data/YataiDON to /data/YataiDON on the console.\n"
        "This source checkout may contain empty skin submodules. Add a complete\n"
        "skin and its parent dependencies under /data/YataiDON/Skins before playing.\n"
        "Configuration, scores and logs are written under /data/YataiDON.\n"
        "Online services, automatic skin downloads, MIDI and webcam are disabled.\n"
        "Audio uses 48 kHz stereo native AudioOut; calibrate latency on hardware.\n")
    run(host_tool, "self", "--inspect", "--file", app / "eboot.bin")
    receipt = {"hardware_tested": False, "title_id": "PPSA99329",
               "runtime_sha256": digest(runtime),
               "graphics_manifest_sha256": digest(prefix / "manifest.sha256"),
               "game_archive_sha256": digest(build / "libyataidon_ps5_game.a"),
               "ffmpeg_sha256": {name: digest(ffmpeg / f"lib/lib{name}.a")
                                  for name in ("avformat", "avcodec", "swscale", "swresample", "avutil", "opus")},
               "files": {p.relative_to(app).as_posix(): digest(p) for p in sorted(app.rglob("*")) if p.is_file()}}
    (ROOT / "dist/build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    licenses = ROOT / "dist/LICENSES"
    licenses.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT.parent / "LICENSE", licenses / "YataiDON-GPL-3.0.txt")
    shutil.copy2(graphics / "LICENSE", licenses / "PS5-OpenGL-GPL-3.0.txt")
    for dependency in (ROOT / "vendor").iterdir():
        if dependency.is_dir():
            for license_file in dependency.iterdir():
                if license_file.is_file() and license_file.name.upper().startswith(("LICENSE", "COPYING")):
                    shutil.copy2(license_file, licenses / f"{dependency.name}-{license_file.name}")
    bundle = ROOT / "dist/YataiDON-PS5-experimental.zip"
    with zipfile.ZipFile(bundle, "w", compression=zipfile.ZIP_DEFLATED) as output:
        directories = (app, data / "shader", licenses) if args.skip_media else (app, data, licenses)
        for directory in directories:
            for path in sorted(directory.rglob("*")):
                if path.is_file():
                    output.write(path, path.relative_to(ROOT / "dist").as_posix())
        for name in ("INSTALL.txt", "build-receipt.json"):
            output.write(ROOT / "dist" / name, name)
    print(f"Native folder: {app}\neboot SHA-256: {digest(app / 'eboot.bin')}")


if __name__ == "__main__":
    main()
