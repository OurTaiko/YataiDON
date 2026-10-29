#pragma once
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "platform.h"

// A single worker owns the decoder and copies output before reusing native pools.
class PS5HardwareVideo : public PlatformVideoDecoder {
    struct Impl;
    std::unique_ptr<Impl> impl;
    explicit PS5HardwareVideo(std::unique_ptr<Impl> value);

public:
    ~PS5HardwareVideo() override;
    static std::unique_ptr<PS5HardwareVideo> open(const std::filesystem::path&, const std::atomic<bool>& stop);
    bool next(std::vector<uint8_t>& rgba, int& width, int& height) override;
};
void ps5_preload_video_decoder();
