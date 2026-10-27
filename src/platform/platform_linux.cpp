#include "platform_linux.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../libs/gfx.h"
#include "../libs/gfx/sdl_compat.h"

#if !defined(__ANDROID__) && !defined(YATAIDON_PLATFORM_IOS) && !defined(__EMSCRIPTEN__)
#include <cpptrace/cpptrace.hpp>
#endif

namespace {

    void write_all(int fd, const char* buf, std::size_t len) {
        while (len > 0) {
            ssize_t n = write(fd, buf, len);
            if (n > 0) {
                buf += n;
                len -= static_cast<std::size_t>(n);
                continue;
            }
            if (n < 0 && errno == EINTR) {
                continue;
            }
            break;
        }
    }

#define CRASH_WRITE_LIT(fd, lit) write_all((fd), (lit), sizeof(lit) - 1)

    void write_hex(int fd, std::uintptr_t value) {
        char buf[2 + sizeof(value) * 2];
        buf[0] = '0';
        buf[1] = 'x';
        for (std::size_t i = 0; i < sizeof(value) * 2; i++) {
            int nibble = (value >> (4 * (sizeof(value) * 2 - 1 - i))) & 0xF;
            buf[2 + i] = nibble < 10 ? static_cast<char>('0' + nibble) : static_cast<char>('a' + nibble - 10);
        }
        write_all(fd, buf, sizeof(buf));
    }

    void crash_signal_handler(int sig) {
        switch (sig) {
        case SIGSEGV:
            CRASH_WRITE_LIT(STDERR_FILENO, "Crash: SIGSEGV (Segmentation fault)\n");
            break;
        case SIGABRT:
            CRASH_WRITE_LIT(STDERR_FILENO, "Crash: SIGABRT (Abort)\n");
            break;
        case SIGFPE:
            CRASH_WRITE_LIT(STDERR_FILENO, "Crash: SIGFPE (Floating point exception)\n");
            break;
        case SIGILL:
            CRASH_WRITE_LIT(STDERR_FILENO, "Crash: SIGILL (Illegal instruction)\n");
            break;
        default:
            CRASH_WRITE_LIT(STDERR_FILENO, "Unknown signal\n");
            break;
        }
#if !defined(__ANDROID__) && !defined(YATAIDON_PLATFORM_IOS) && !defined(__EMSCRIPTEN__)
        cpptrace::frame_ptr frames[64];
        std::size_t count = cpptrace::safe_generate_raw_trace(frames, 64);
        for (std::size_t i = 0; i < count; i++) {
            write_hex(STDERR_FILENO, frames[i]);
            CRASH_WRITE_LIT(STDERR_FILENO, "\n");
        }
#endif
        _exit(1);
    }

    void signal_handler(int signal) {
        if (signal == SIGINT) {
            _exit(0);
        }
    }

} // namespace

std::filesystem::path unix_get_executable_dir() {
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        spdlog::error("Failed to get executable path");
        return {};
    }
    buffer[len] = '\0';
    return std::filesystem::path(buffer).parent_path();
}

void unix_close_window() {
    // SDL unloads EGL before disconnecting Wayland; queued NVIDIA events still
    // reference this module's protocol metadata, so keep it loaded until then.
    void* egl_wayland = nullptr;
    const char* driver = SDL_GetCurrentVideoDriver();

    if (driver && std::strcmp(driver, "wayland") == 0) {
        egl_wayland = dlopen("libnvidia-egl-wayland.so.1", RTLD_LAZY | RTLD_NOLOAD);
    }

    close_window();

    if (egl_wayland) {
        dlclose(egl_wayland);
    }
}

void unix_install_crash_handlers() {
    // Run the crash handler on its own stack so a stack-overflow SIGSEGV
    // (where the normal stack is exhausted) still gets caught. On modern
    // glibc SIGSTKSZ is not a compile-time constant and is too small for
    // spdlog formatting + cpptrace symbolisation, so enforce a floor.
    static std::vector<char> altstack(std::max<std::size_t>(SIGSTKSZ, 64 * 1024));
    stack_t ss{};
    ss.ss_sp = altstack.data();
    ss.ss_size = altstack.size();
    ss.ss_flags = 0;
    if (sigaltstack(&ss, nullptr) != 0) {
        spdlog::warn("sigaltstack failed: {}", strerror(errno));
    }

    struct sigaction sa{};
    sa.sa_handler = crash_signal_handler;
    sa.sa_flags = SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, nullptr) != 0) {
        spdlog::warn("sigaction(SIGSEGV) failed: {}", strerror(errno));
    }
    if (sigaction(SIGABRT, &sa, nullptr) != 0) {
        spdlog::warn("sigaction(SIGABRT) failed: {}", strerror(errno));
    }
    if (sigaction(SIGFPE, &sa, nullptr) != 0) {
        spdlog::warn("sigaction(SIGFPE) failed: {}", strerror(errno));
    }
    if (sigaction(SIGILL, &sa, nullptr) != 0) {
        spdlog::warn("sigaction(SIGILL) failed: {}", strerror(errno));
    }

    // Use sigaction (not std::signal) for consistent restart/mask semantics
    // with the crash handlers above.
    struct sigaction sa_int{};
    sa_int.sa_handler = signal_handler;
    sigemptyset(&sa_int.sa_mask);
    if (sigaction(SIGINT, &sa_int, nullptr) != 0) {
        spdlog::warn("sigaction(SIGINT) failed: {}", strerror(errno));
    }
}
