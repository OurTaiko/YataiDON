// GPL-3.0-or-later. Native ABI/call sequence from EVO Player (sainsaji),
// commit 875f766dc9de762c1a1f608005d0afd1f25116f4, itself based on ProsperoLight.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>

#include "../libs/video/av.h"
#include "ps5_h264_annexb.h"
#include "ps5_video.h"
#include "ps5_videodec2_abi.h"
static_assert(sizeof(SceVideodec2DecoderConfigInfo) == 72);
static_assert(sizeof(SceVideodec2DecoderMemoryInfo) == 72);
static_assert(sizeof(SceVideodec2InputData) == 48);
static_assert(sizeof(SceVideodec2FrameBuffer) == 32);
static_assert(sizeof(SceVideodec2OutputInfo) == 56);

extern "C" {
int sceSysmoduleLoadModule(uint16_t);
int sceKernelGetModuleList(int*, size_t, size_t*);
int sceKernelDlsym(int, const char*, void**);
int sceKernelLoadStartModule(const char*, size_t, const void*, unsigned, void*, int*);
int64_t sceKernelGetDirectMemorySize();
int sceKernelAllocateDirectMemory(int64_t, int64_t, size_t, size_t, int, int64_t*);
int sceKernelMapDirectMemory(void**, size_t, int, int, int64_t, size_t);
int sceKernelReleaseDirectMemory(int64_t, size_t);
int sceKernelMunmap(void*, size_t);
int sceKernelMapNamedFlexibleMemory(void**, size_t, int, int, const char*);
int sceKernelReleaseFlexibleMemory(void*, size_t);
}

namespace {
    struct Api {
#define PS5_VDEC_SYMBOL(name, nid) decltype(&sceVideodec2##name) name = nullptr;
#include "ps5_videodec_symbols.inc"
#undef PS5_VDEC_SYMBOL
        bool ready = false;
    };
    Api api;
    std::once_flag api_once;
    // One compute decoder at a time, without blocking a second video worker.
    std::mutex decoder_owner;
    size_t aligned(size_t n) {
        if (n > (512u << 20)) {
            throw std::runtime_error("Excessive video decoder allocation");
        }
        return (n + 0x3fff) & ~size_t(0x3fff);
    }
    void check(int rc, const char* stage) {
        if (rc != 0) {
            std::fprintf(stderr, "[ps5-vdec] %s failed=%08x\n", stage, unsigned(rc));
            throw std::runtime_error(stage);
        }
    }
    struct Direct {
        void* data = nullptr;
        int64_t offset = -1;
        size_t size = 0;
        void allocate(size_t bytes) {
            size = aligned(bytes);
            if (!size) {
                return;
            }
            check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), size, 0x4000, 12, &offset),
                  "allocate direct");
            check(sceKernelMapDirectMemory(&data, size, 0x33, 0, offset, 0x4000), "map direct");
        }
        ~Direct() {
            if (data) {
                sceKernelMunmap(data, size);
            }
            if (offset >= 0) {
                sceKernelReleaseDirectMemory(offset, size);
            }
        }
    };
} // namespace

void ps5_preload_video_decoder() {
    std::call_once(api_once, [] {
        int rc = sceSysmoduleLoadModule(SCE_SYSMODULE_VIDEODEC2);
        // A module already loaded by the shell may reject a duplicate load;
        // still look for its exports before choosing software fallback.
        if (rc != 0) {
            std::fprintf(stderr, "[ps5-vdec] module load result=%08x\n", unsigned(rc));
        }
        // Import through the native title's module table, like AudioOut/AGC.
        // The kernel resolves these symbols in the registered game's namespace;
        // loading an absolute firmware pathname does not work from that sandbox.
#define PS5_VDEC_SYMBOL(name, nid) api.name = &sceVideodec2##name;
#include "ps5_videodec_symbols.inc"
#undef PS5_VDEC_SYMBOL
        api.ready = true;
        std::fprintf(stderr, "[ps5-vdec] hardware H.264 API %s\n",
                     api.ready ? "ready" : "unavailable; software fallback");
    });
}

struct PS5HardwareVideo::Impl {
    std::unique_lock<std::mutex> owner{decoder_owner, std::try_to_lock};
    const std::atomic<bool>* stop = nullptr;
    AVFormatContext* format = nullptr;
    AVPacket* packet = nullptr;
    int stream = -1, width = 0, height = 0;
    PS5H264AnnexB annexb;
    std::vector<uint8_t> access_unit;
    Direct compute, gpu, cpu_gpu, input, frames;
    void* queue = nullptr;
    void* decoder = nullptr;
    void* cpu = nullptr;
    size_t cpu_size = 0, frame_size = 0;
    unsigned ring = 0;
    bool eof = false;
    unsigned flush_count = 0;
    unsigned output_frames = 0;
    SwsContext* converter = nullptr;
    static constexpr size_t au_size = 8u << 20;
    ~Impl() {
        if (decoder) {
            api.DeleteDecoder(decoder);
        }
        if (queue) {
            api.ReleaseComputeQueue(queue);
        }
        if (cpu) {
            sceKernelReleaseFlexibleMemory(cpu, cpu_size);
            sceKernelMunmap(cpu, cpu_size);
        }
        if (converter) {
            sws_freeContext(converter);
        }
        av_packet_free(&packet);
        avformat_close_input(&format);
    }
    void initialize(const std::filesystem::path& path, const std::atomic<bool>& stopping) {
        if (!owner.owns_lock() || !api.ready) {
            throw std::runtime_error("Hardware decoder unavailable or busy");
        }
        stop = &stopping;
        format = avformat_alloc_context();
        if (!format) {
            throw std::bad_alloc();
        }
        format->interrupt_callback = {[](void* flag) -> int {
                                          return static_cast<const std::atomic<bool>*>(flag)->load(
                                              std::memory_order_relaxed);
                                      },
                                      const_cast<std::atomic<bool>*>(stop)};
        av::check(avformat_open_input(&format, path.string().c_str(), nullptr, nullptr), "open hardware video");
        av::check(avformat_find_stream_info(format, nullptr), "probe hardware video");
        for (unsigned i = 0; i < format->nb_streams; ++i) {
            if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                stream = int(i);
                break;
            }
        }
        if (stream < 0) {
            throw std::runtime_error("No video stream");
        }
        auto* p = format->streams[stream]->codecpar;
        int profile = p->profile & 255;
        if (p->codec_id != AV_CODEC_ID_H264 || (profile != 66 && profile != 77 && profile != 100) || p->width <= 0 ||
            p->height <= 0 || p->width > 3840 || p->height > 2160 ||
            (p->format != -1 && p->format != AV_PIX_FMT_YUV420P)) {
            throw std::runtime_error("Stream requires software video decoding");
        }
        width = p->width;
        height = p->height;
        annexb.configure(p->extradata, size_t(p->extradata_size));
        packet = av_packet_alloc();
        if (!packet) {
            throw std::bad_alloc();
        }
        SceVideodec2ComputeMemoryInfo cm{};
        cm.size = sizeof(cm);
        check(api.QueryComputeMemoryInfo(&cm), "query compute");
        compute.allocate(cm.cpu_gpu_size);
        cm.cpu_gpu_size = compute.size;
        cm.cpu_gpu = compute.data;
        SceVideodec2ComputeConfigInfo cc{};
        cc.size = sizeof(cc);
        check(api.AllocateComputeQueue(&cc, &cm, &queue), "create compute queue");
        SceVideodec2DecoderConfigInfo cfg{};
        cfg.size = sizeof(cfg);
        cfg.resource_type = 1;
        cfg.codec_type = 1;
        cfg.profile = profile;
        cfg.max_level = std::max(p->level, width > 1920 ? 52 : 51);
        cfg.max_width = (width + 15) & ~15;
        cfg.max_height = (height + 63) & ~63;
        cfg.max_dpb_frames = -1;
        cfg.pipeline_depth = 1;
        cfg.compute_queue = reinterpret_cast<uint64_t>(queue);
        cfg.cpu_affinity = 0x3f;
        cfg.cpu_priority = 700;
        cfg.optimize_progressive = 1;
        SceVideodec2DecoderMemoryInfo mem{};
        mem.size = sizeof(mem);
        check(api.QueryDecoderMemoryInfo(&cfg, &mem), "query decoder memory");
        cpu_size = aligned(mem.cpu_size);
        if (cpu_size) {
            check(sceKernelMapNamedFlexibleMemory(&cpu, cpu_size, 3, 0, "YataiVideo"), "map CPU workspace");
        }
        gpu.allocate(mem.gpu_size);
        cpu_gpu.allocate(mem.cpu_gpu_size);
        frame_size = aligned(mem.max_frame_size);
        if (!frame_size) {
            throw std::runtime_error("Empty hardware video frame allocation");
        }
        input.allocate(au_size * 3);
        frames.allocate(frame_size * 8);
        mem.cpu = cpu;
        mem.cpu_size = cpu_size;
        mem.gpu = gpu.data;
        mem.gpu_size = gpu.size;
        mem.cpu_gpu = cpu_gpu.data;
        mem.cpu_gpu_size = cpu_gpu.size;
        check(api.CreateDecoder(&cfg, &mem, &decoder), "create H.264 decoder");
        check(api.Reset(decoder), "reset H.264 decoder");
        std::fprintf(stderr, "[ps5-vdec] hardware H.264 %dx%d workspace=%zu MiB\n", width, height,
                     (compute.size + gpu.size + cpu_gpu.size + input.size + frames.size + cpu_size) >> 20);
    }
    bool next(std::vector<uint8_t>& rgba, int& out_width, int& out_height) {
        while (!stop->load(std::memory_order_relaxed)) {
            SceVideodec2FrameBuffer fb{};
            fb.size = sizeof(fb);
            fb.buffer = static_cast<uint8_t*>(frames.data) + (ring % 8) * frame_size;
            fb.buffer_size = frame_size;
            SceVideodec2OutputInfo out{};
            out.size = sizeof(out);
            int rc;
            if (eof) {
                if (++flush_count > 32) {
                    throw std::runtime_error("Hardware video flush limit exceeded");
                }
                rc = api.Flush(decoder, &fb, &out);
            } else {
                int read;
                do {
                    av_packet_unref(packet);
                    read = av_read_frame(format, packet);
                } while (read >= 0 && packet->stream_index != stream && !stop->load(std::memory_order_relaxed));
                if (read < 0) {
                    if (read != AVERROR_EOF && !stop->load()) {
                        throw std::runtime_error("Hardware video read failed");
                    }
                    eof = true;
                    continue;
                }
                annexb.convert(packet->data, size_t(packet->size), (packet->flags & AV_PKT_FLAG_KEY) != 0, access_unit);
                if (access_unit.empty()) {
                    continue;
                }
                if (access_unit.size() > au_size) {
                    throw std::runtime_error("Hardware video AU exceeds pool slot");
                }
                void* bytes = static_cast<uint8_t*>(input.data) + (ring % 3) * au_size;
                std::memcpy(bytes, access_unit.data(), access_unit.size());
                SceVideodec2InputData in{};
                in.size = sizeof(in);
                in.au = bytes;
                in.au_size = access_unit.size();
                in.pts = packet->pts;
                in.dts = UINT64_MAX;
                rc = api.Decode(decoder, &in, &fb, &out);
            }
            ++ring;
            check(rc, eof ? "flush H.264" : "decode H.264");
            if (out.error) {
                throw std::runtime_error("Hardware video reported a damaged frame");
            }
            if (!out.valid) {
                if (eof) {
                    return false;
                }
                continue;
            }
            const size_t pitch = out.pitch_bytes ? out.pitch_bytes : out.pitch;
            const size_t bytes = pitch * out.height * 3 / 2;
            const uintptr_t start = reinterpret_cast<uintptr_t>(out.buffer);
            const uintptr_t pool = reinterpret_cast<uintptr_t>(frames.data);
            if (pitch < size_t(width) || pitch > 8192 || out.width < unsigned(width) || out.width > 3840 ||
                (out.pitch_bytes && out.pitch_bytes != out.pitch) || out.height < unsigned(height) ||
                out.height > 2176 || !out.buffer || start < pool || start - pool > frames.size ||
                bytes > frames.size - (start - pool)) {
                throw std::runtime_error("Invalid hardware NV12 output bounds");
            }
            converter = sws_getCachedContext(converter, width, height, AV_PIX_FMT_NV12, width, height, AV_PIX_FMT_RGBA,
                                             SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!converter) {
                throw std::runtime_error("Hardware video colour converter failed");
            }
            rgba.resize(size_t(width) * height * 4);
            const uint8_t* src[4] = {static_cast<uint8_t*>(out.buffer),
                                     static_cast<uint8_t*>(out.buffer) + pitch * out.height};
            int stride[4] = {int(pitch), int(pitch), 0, 0};
            uint8_t* dst[4] = {rgba.data()};
            int dst_stride[4] = {width * 4};
            if (sws_scale(converter, src, stride, 0, height, dst, dst_stride) != height) {
                throw std::runtime_error("Hardware video conversion incomplete");
            }
            out_width = width;
            out_height = height;
            if (++output_frames <= 3) {
                std::fprintf(stderr, "[ps5-vdec] hardware frame=%u %dx%d\n", output_frames, width, height);
            }
            return true;
        }
        return false;
    }
};

PS5HardwareVideo::PS5HardwareVideo(std::unique_ptr<Impl> value) : impl(std::move(value)) {}
PS5HardwareVideo::~PS5HardwareVideo() = default;
std::unique_ptr<PS5HardwareVideo> PS5HardwareVideo::open(const std::filesystem::path& path,
                                                         const std::atomic<bool>& stop) {
    try {
        ps5_preload_video_decoder();
        auto state = std::make_unique<Impl>();
        state->initialize(path, stop);
        return std::unique_ptr<PS5HardwareVideo>(new PS5HardwareVideo(std::move(state)));
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[ps5-vdec] %s; software fallback\n", error.what());
        return {};
    }
}
bool PS5HardwareVideo::next(std::vector<uint8_t>& rgba, int& width, int& height) {
    return impl->next(rgba, width, height);
}
