#include "../src/platform/ps5_h264_annexb.h"
#include <cassert>
int main() {
    PS5H264AnnexB avc;
    uint8_t config[] = {1,100,0,31,255,225,0,2,0x67,1,1,0,2,0x68,2};
    avc.configure(config, sizeof(config));
    assert(avc.length_bytes == 4 && avc.headers.size() == 12);
    uint8_t packet[] = {0,0,0,2,0x65,3,0,0,0,1,0x61};
    std::vector<uint8_t> output;
    avc.convert(packet, sizeof(packet), true, output);
    assert(output.size() == 23 && output[16] == 0x65 && output[22] == 0x61);
    avc.convert(packet, sizeof(packet), false, output);
    assert(output.size() == 11);
    bool rejected = false;
    try { avc.convert(packet, 5, false, output); } catch (...) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { PS5H264AnnexB bad; bad.configure(config, 8); } catch (...) { rejected = true; }
    assert(rejected);
    PS5H264AnnexB raw;
    uint8_t annexb[] = {0,0,0,1,0x65,0x12};
    raw.convert(annexb, sizeof(annexb), false, output);
    assert(output == std::vector<uint8_t>(annexb, annexb + sizeof(annexb)));
}
