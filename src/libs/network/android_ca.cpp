#include "android_ca.h"

#if defined(__ANDROID__)

#include <fstream>
#include <spdlog/spdlog.h>
#include <string>

#include "../gfx/sdl_compat.h"

namespace {

    std::string extract_bundled_cacert(const std::string& out_path) {
        SDL_IOStream* io = SDL_IOFromFile("cacert.pem", "r");
        if (!io) {
            spdlog::error("Failed to open bundled cacert.pem asset");
            return std::string{};
        }
        Sint64 size = SDL_GetIOSize(io);
        if (size <= 0) {
            SDL_CloseIO(io);
            return std::string{};
        }
        std::string buf(static_cast<std::size_t>(size), '\0');
        const std::size_t read = SDL_ReadIO(io, buf.data(), static_cast<std::size_t>(size));
        SDL_CloseIO(io);
        if (read != static_cast<std::size_t>(size)) {
            spdlog::error("Truncated read of bundled cacert.pem ({} of {} bytes)", read, size);
            return std::string{};
        }

        std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
        if (!out) {
            spdlog::error("Failed to write cacert.pem to {}", out_path);
            return std::string{};
        }
        out << buf;
        return out_path;
    }

    // libcurl on Android has no system CA store to fall back on; extract the
    // bundled Mozilla cacert.pem to a real path once and point every request's
    // CURLOPT_CAINFO at it.
    std::string ca_bundle_path() {
        static const std::string path = [] {
            // Store in app-private storage; external storage (/sdcard/...) is
            // writable by other apps, which would let them substitute a rogue
            // trust anchor for every HTTPS request.
            const std::string out_path = std::string(SDL_GetPrefPath("YataiDON", "certs")) + "cacert.pem";
            std::ifstream existing(out_path, std::ios::binary | std::ios::ate);
            // A truncated/empty file left over from an interrupted previous
            // extraction must not be trusted as-is -- re-extract instead.
            if (existing.good() && existing.tellg() > 0) {
                return out_path;
            }

            return extract_bundled_cacert(out_path);
        }();
        return path;
    }

} // namespace

cpr::SslOptions android_ca() {
    const std::string& path = ca_bundle_path();
    if (path.empty()) {
        spdlog::error("No CA bundle available; TLS verification cannot be configured");
    }
    return cpr::Ssl(cpr::ssl::CaInfo{path}, cpr::ssl::VerifyPeer{true}, cpr::ssl::VerifyHost{true});
}

#endif
