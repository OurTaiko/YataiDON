#pragma once
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

// A single worker owns the decoder and copies output before reusing native pools.
class PS5HardwareVideo {
    struct Impl;
    std::unique_ptr<Impl> impl;
    explicit PS5HardwareVideo(std::unique_ptr<Impl> value);
public:
    ~PS5HardwareVideo();
    static std::unique_ptr<PS5HardwareVideo> open(const std::filesystem::path&, const std::atomic<bool>& stop);
    bool next(std::vector<uint8_t>& rgba, int& width, int& height);
};
void ps5_preload_video_decoder();
