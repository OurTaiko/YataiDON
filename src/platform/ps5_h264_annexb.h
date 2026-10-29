#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

// MP4 stores length-prefixed NALs; Videodec2 consumes Annex B access units.
struct PS5H264AnnexB {
    unsigned length_bytes = 0;
    std::vector<uint8_t> headers;
    static void nal(std::vector<uint8_t>& out, const uint8_t* bytes, size_t size) {
        out.insert(out.end(), {0, 0, 0, 1});
        out.insert(out.end(), bytes, bytes + size);
    }
    void configure(const uint8_t* bytes, size_t size) {
        if (!size) {
            return;
        }
        if (bytes[0] != 1) {
            headers.assign(bytes, bytes + size);
            return;
        }
        if (size < 7) {
            throw std::runtime_error("Truncated avcC header");
        }
        length_bytes = (bytes[4] & 3) + 1;
        size_t at = 6;
        unsigned count = bytes[5] & 31;
        for (int group = 0; group < 2; ++group) {
            for (unsigned i = 0; i < count; ++i) {
                if (at + 2 > size) {
                    throw std::runtime_error("Truncated avcC NAL length");
                }
                size_t n = (size_t(bytes[at]) << 8) | bytes[at + 1];
                at += 2;
                if (!n || n > size - at) {
                    throw std::runtime_error("Invalid avcC NAL");
                }
                nal(headers, bytes + at, n);
                at += n;
            }
            if (!group) {
                if (at >= size) {
                    throw std::runtime_error("Missing avcC PPS count");
                }
                count = bytes[at++];
            }
        }
    }
    void convert(const uint8_t* bytes, size_t size, bool key, std::vector<uint8_t>& out) const {
        out.clear();
        if (key) {
            out.insert(out.end(), headers.begin(), headers.end());
        }
        if (!length_bytes) {
            out.insert(out.end(), bytes, bytes + size);
            return;
        }
        for (size_t at = 0; at < size;) {
            if (size - at < length_bytes) {
                throw std::runtime_error("Truncated H.264 NAL length");
            }
            size_t n = 0;
            for (unsigned i = 0; i < length_bytes; ++i) {
                n = (n << 8) | bytes[at++];
            }
            if (!n || n > size - at) {
                throw std::runtime_error("Invalid H.264 NAL size");
            }
            nal(out, bytes + at, n);
            at += n;
        }
    }
};
