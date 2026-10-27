#include "gfx.h"

#include <array>
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "gfx/sdl_compat.h"
#include "shaders/spirv/fs_2d.sc.bin.h"
#include "shaders/spirv/fs_colortransform.sc.bin.h"
#include "shaders/spirv/vs_2d.sc.bin.h"

namespace {

    // raylib enum values the game passes through gfx.h
    constexpr int FILTER_POINT = 0, FILTER_BILINEAR = 1;
    constexpr int WRAP_REPEAT = 0, WRAP_CLAMP = 1, WRAP_MIRROR = 2;
    constexpr int BLEND_ALPHA = 0, BLEND_MULTIPLIED = 2, BLEND_ADD_COLORS = 3, BLEND_SUBTRACT_COLORS = 4,
                  BLEND_ALPHA_PREMULTIPLY = 5;
    constexpr int TEXT_LINE_SPACING = 2; // raylib's default, matches measure_text_ex

    struct Vertex {
        float x, y, z, u, v;
        uint32_t rgba; // bytes r,g,b,a in memory
    };

    struct TextureSlot {
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
        int filter = FILTER_POINT; // raylib's defaults
        int wrap = WRAP_REPEAT;
    };

    struct ProgramSlot {
        bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
        std::vector<std::string> names; // index = shader "location"
        std::vector<bgfx::UniformHandle> uniforms;
        std::vector<std::array<float, 4>> values;
    };

    struct TargetSlot {
        bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE;
        unsigned color = 0; // texture id
        int width = 0, height = 0;
    };

    struct Pass {
        bgfx::ViewId view = 0;
        unsigned target = 0; // render target id, 0 = screen
        int width = 0, height = 0;
        bool flip = false; // RT stored bottom-up like GL so the game's negative-height sampling holds
        bool clear = false;
        unsigned submits = 0;
    };

    struct Scissor {
        bool on = false;
        uint16_t x = 0, y = 0, w = 0, h = 0;
        bool operator==(const Scissor&) const = default;
    };

    SDL_Window* window = nullptr;
    unsigned window_flags = 0;
    bool should_close = false;
    uint32_t reset_flags = BGFX_RESET_NONE;
    const bgfx::Caps* caps = nullptr;

    bgfx::VertexLayout layout;
    bgfx::UniformHandle s_tex_color = BGFX_INVALID_HANDLE;
    unsigned white_texture = 0;
    Font default_font;

    // id = index + 1, so id 0 stays "none"
    std::vector<TextureSlot> textures;
    std::vector<ProgramSlot> programs; // [0] = default 2D program
    std::vector<TargetSlot> targets;

    // frame / pass state
    bool frame_open = false;
    bgfx::ViewId next_view = 0;
    Pass pass;

    // Texture data queued for the next bgfx::frame(). bgfx uploads it all inside that call,
    // so a screen that loads hundreds of textures would hitch on its first frame.
    size_t pending_upload_bytes = 0;
    constexpr size_t UPLOAD_FLUSH_BYTES = 32u << 20;
    unsigned submits_this_frame = 0;
    bool screen_drawn = false; // a screen view this frame has a clear or a draw

    void flush_uploads_if_idle() {
        // Screens load in update(), after begin_drawing() but before any draw. While nothing is
        // submitted yet, FLUSH runs the queued uploads (and any pending clears) without presenting,
        // so the cost stays in the loading frame, as with raylib's glTexImage2D.
        if (pending_upload_bytes >= UPLOAD_FLUSH_BYTES && submits_this_frame == 0) {
            bgfx::frame(BGFX_FRAME_FLUSH);
            pending_upload_bytes = 0;
        }
    }

    // batch state
    std::vector<Vertex> verts;
    unsigned cur_texture = 0;
    unsigned cur_program = 0; // index into programs
    bool cur_lines = false;
    uint64_t cur_blend = 0;
    uint64_t custom_separate_blend = 0;
    Scissor cur_scissor;
    std::array<float, 16> cur_model{};

    constexpr std::array<float, 16> identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // Reuses the first dead slot (per `alive`) or appends; returns index + 1.
    template<class T, class Alive>
    unsigned add_slot(std::vector<T>& slots, const T& slot, Alive alive) {
        for (size_t i = 0; i < slots.size(); i++) {
            if (!alive(slots[i])) {
                slots[i] = slot;
                return (unsigned)i + 1;
            }
        }
        slots.push_back(slot);
        return (unsigned)slots.size();
    }
    unsigned add_slot(const TextureSlot& t) {
        return add_slot(textures, t, [](const TextureSlot& s) { return bgfx::isValid(s.handle); });
    }
    unsigned add_slot(const TargetSlot& t) {
        return add_slot(targets, t, [](const TargetSlot& s) { return bgfx::isValid(s.fb); });
    }
    unsigned add_program(const ProgramSlot& p) { // index; [0] is the default program
        return add_slot(programs, p, [](const ProgramSlot& s) { return bgfx::isValid(s.program); }) - 1;
    }

    void window_pixels(int& w, int& h) {
        SDL_GetWindowSizeInPixels(window, &w, &h);
    }

    TextureSlot* texture_slot(unsigned id) {
        return id && id <= textures.size() && bgfx::isValid(textures[id - 1].handle) ? &textures[id - 1] : nullptr;
    }

    uint64_t blend_state(int mode) {
        switch (mode) {
        case blend_mode::ADDITIVE:
            return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);
        case BLEND_MULTIPLIED:
            return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_DST_COLOR, BGFX_STATE_BLEND_INV_SRC_ALPHA);
        case BLEND_ADD_COLORS:
            return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
        case BLEND_SUBTRACT_COLORS:
            return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) |
                   BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_SUB);
        case BLEND_ALPHA_PREMULTIPLY:
            return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
        case blend_mode::CUSTOM_SEPARATE:
            return custom_separate_blend;
        default:
            return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
        }
    }

    uint32_t sampler_flags(const TextureSlot& t) {
        uint32_t f = 0;
        if (t.filter == FILTER_POINT) {
            f |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
        } else if (t.filter == FILTER_BILINEAR) {
            f |= BGFX_SAMPLER_MIP_POINT;
        } // trilinear and anisotropic: bgfx's default linear/linear/linear
        if (t.wrap == WRAP_CLAMP) {
            f |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        } else if (t.wrap == WRAP_MIRROR) {
            f |= BGFX_SAMPLER_U_MIRROR | BGFX_SAMPLER_V_MIRROR;
        }
        return f;
    }

    void flush() {
        if (verts.empty()) {
            return;
        }
        const auto n = (uint32_t)verts.size();
        if (bgfx::getAvailTransientVertexBuffer(n, layout) < n) {
            static bool warned = false;
            if (!warned) {
                spdlog::warn("bgfx: transient vertex buffer full, dropping draws");
                warned = true;
            }
            verts.clear();
            return;
        }
        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, n, layout);
        std::memcpy(tvb.data, verts.data(), n * sizeof(Vertex));
        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setTransform(cur_model.data());
        const TextureSlot* tex = texture_slot(cur_texture);
        if (!tex) {
            tex = texture_slot(white_texture);
        }
        bgfx::setTexture(0, s_tex_color, tex->handle, sampler_flags(*tex));
        if (cur_scissor.on) {
            bgfx::setScissor(cur_scissor.x, cur_scissor.y, cur_scissor.w, cur_scissor.h);
        }
        ProgramSlot& prog = programs[cur_program];
        for (size_t i = 0; i < prog.uniforms.size(); i++) {
            bgfx::setUniform(prog.uniforms[i], prog.values[i].data());
        }
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | cur_blend | (cur_lines ? BGFX_STATE_PT_LINES : 0));
        bgfx::submit(pass.view, prog.program);
        submits_this_frame++;
        pass.submits++;
        screen_drawn |= pass.target == 0;
        verts.clear();
    }

    // Starts a new view on `target` (0 = screen). Draws before it stay in earlier views.
    void open_pass(unsigned target, bool clear = false, Color clear_color = {}) {
        flush();
        // A view still empty (no draw, no clear) is reconfigured instead of spending a new id.
        const bool reuse = frame_open && pass.submits == 0 && !pass.clear;
        if (!frame_open) {
            next_view = 0;
            frame_open = true;
        }
        if (reuse) {
        } else if (next_view >= caps->limits.maxViews - 1) {
            static bool warned = false;
            if (!warned) {
                spdlog::warn("bgfx: out of views this frame; later passes share the last one");
                warned = true;
            }
        } else {
            pass.view = next_view++;
        }
        pass.target = target;
        pass.clear = clear;
        pass.submits = 0;
        if (const TargetSlot* t = target ? &targets[target - 1] : nullptr) {
            pass.width = t->width;
            pass.height = t->height;
            pass.flip = !caps->originBottomLeft;
        } else {
            window_pixels(pass.width, pass.height);
            pass.flip = false;
        }
        bgfx::setViewMode(pass.view, bgfx::ViewMode::Sequential);
        bgfx::setViewFrameBuffer(pass.view,
                                 target ? targets[target - 1].fb : bgfx::FrameBufferHandle BGFX_INVALID_HANDLE);
        bgfx::setViewRect(pass.view, 0, 0, (uint16_t)pass.width, (uint16_t)pass.height);
        bgfx::setViewClear(pass.view, clear ? BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH : BGFX_CLEAR_NONE,
                           (uint32_t)clear_color.r << 24 | (uint32_t)clear_color.g << 16 |
                               (uint32_t)clear_color.b << 8 | clear_color.a,
                           1.0f, 0);
        float proj[16];
        const float bottom = pass.flip ? 0.0f : (float)pass.height;
        const float top = pass.flip ? (float)pass.height : 0.0f;
        bx::mtxOrtho(proj, 0.0f, (float)pass.width, bottom, top, -1.0f, 1.0f, 0.0f, caps->homogeneousDepth);
        bgfx::setViewTransform(pass.view, nullptr, proj);
        if (clear) {
            bgfx::touch(pass.view); // so the clear happens even without draws
            screen_drawn |= target == 0;
        }
        cur_scissor = {};
    }

    void use(unsigned texture, bool lines) {
        if (texture != cur_texture || lines != cur_lines) {
            flush();
            cur_texture = texture;
            cur_lines = lines;
        }
    }

    uint32_t pack(Color c) {
        return (uint32_t)c.r | (uint32_t)c.g << 8 | (uint32_t)c.b << 16 | (uint32_t)c.a << 24;
    }

    void push_quad(Vec2 tl, Vec2 bl, Vec2 br, Vec2 tr, Vec2 uv_tl, Vec2 uv_bl, Vec2 uv_br, Vec2 uv_tr, Color c) {
        const uint32_t col = pack(c);
        const Vertex a{tl.x, tl.y, 0, uv_tl.x, uv_tl.y, col}, b{bl.x, bl.y, 0, uv_bl.x, uv_bl.y, col},
            d{br.x, br.y, 0, uv_br.x, uv_br.y, col}, e{tr.x, tr.y, 0, uv_tr.x, uv_tr.y, col};
        verts.insert(verts.end(), {a, b, d, a, d, e});
    }

    void push_triangle(Vec2 a, Vec2 b, Vec2 c, Color color) {
        const uint32_t col = pack(color);
        verts.insert(verts.end(),
                     {Vertex{a.x, a.y, 0, 0, 0, col}, Vertex{b.x, b.y, 0, 0, 0, col}, Vertex{c.x, c.y, 0, 0, 0, col}});
    }

    void push_line(Vec2 a, Vec2 b, Color c) {
        use(white_texture, true);
        const uint32_t col = pack(c);
        verts.insert(verts.end(), {Vertex{a.x, a.y, 0, 0, 0, col}, Vertex{b.x, b.y, 0, 0, 0, col}});
    }

    void fill_rect(Rect r, Color c) {
        use(white_texture, false);
        push_quad({r.x, r.y}, {r.x, r.y + r.height}, {r.x + r.width, r.y + r.height}, {r.x + r.width, r.y}, {0, 0},
                  {0, 1}, {1, 1}, {1, 0}, c);
    }

    // raylib's automatic segment count for a quarter circle of `radius`
    int corner_segments(int segments, float radius) {
        if (segments >= 1) {
            return segments;
        }
        const float th = acosf(2 * powf(1 - 0.5f / radius, 2) - 1);
        const int s = (int)ceilf((2 * 3.14159265358979f / th) / 4.0f);
        return s > 0 ? s : 4;
    }

    // Annulus sector (inner may be 0) around `center` from `start_deg`, 90 degrees.
    void corner(Vec2 center, float inner, float outer, float start_deg, int segments, Color c) {
        const float step = 90.0f / (float)segments;
        for (int i = 0; i < segments; i++) {
            const float a0 = (start_deg + step * i) * deg2rad, a1 = (start_deg + step * (i + 1)) * deg2rad;
            const Vec2 o0{center.x + cosf(a0) * outer, center.y + sinf(a0) * outer};
            const Vec2 o1{center.x + cosf(a1) * outer, center.y + sinf(a1) * outer};
            const Vec2 i0{center.x + cosf(a0) * inner, center.y + sinf(a0) * inner};
            const Vec2 i1{center.x + cosf(a1) * inner, center.y + sinf(a1) * inner};
            push_triangle(i0, o1, o0, c);
            if (inner > 0.0f) {
                push_triangle(i0, i1, o1, c);
            }
        }
    }

    unsigned upload_texture(Image image, bool mipmaps) {
        Image rgba = image;
        if (image.format != pixel_format::RGBA8) {
            rgba = image_copy(image);
            image_format(&rgba, pixel_format::RGBA8);
        }
        int levels = 1;
        if (mipmaps) {
            for (int w = rgba.width, h = rgba.height; w > 1 || h > 1; w = std::max(1, w / 2), h = std::max(1, h / 2)) {
                levels++;
            }
        }
        TextureSlot slot;
        slot.handle = bgfx::createTexture2D((uint16_t)rgba.width, (uint16_t)rgba.height, levels > 1, 1,
                                            bgfx::TextureFormat::RGBA8);
        // ponytail: box-filtered mips on the CPU; GPU generation if load time ever matters
        const auto* level = static_cast<const uint8_t*>(rgba.data);
        std::vector<uint8_t> next, prev;
        int w = rgba.width, h = rgba.height;
        for (int mip = 0; mip < levels; mip++) {
            const auto bytes = (uint32_t)(w * h * 4);
            bgfx::updateTexture2D(slot.handle, 0, (uint8_t)mip, 0, 0, (uint16_t)w, (uint16_t)h,
                                  bgfx::copy(level, bytes));
            pending_upload_bytes += bytes;
            if (mip + 1 == levels) {
                break;
            }
            const int nw = std::max(1, w / 2), nh = std::max(1, h / 2);
            next.resize((size_t)nw * nh * 4);
            for (int y = 0; y < nh; y++) {
                const int y0 = std::min(y * 2, h - 1), y1 = std::min(y * 2 + 1, h - 1);
                for (int x = 0; x < nw; x++) {
                    const int x0 = std::min(x * 2, w - 1), x1 = std::min(x * 2 + 1, w - 1);
                    for (int c = 0; c < 4; c++) {
                        next[(y * nw + x) * 4 + c] =
                            (uint8_t)((level[(y0 * w + x0) * 4 + c] + level[(y0 * w + x1) * 4 + c] +
                                       level[(y1 * w + x0) * 4 + c] + level[(y1 * w + x1) * 4 + c] + 2) /
                                      4);
                    }
                }
            }
            prev.swap(next);
            level = prev.data();
            w = nw;
            h = nh;
        }
        if (rgba.data != image.data) {
            unload_image(rgba);
        }
        flush_uploads_if_idle();
        return add_slot(slot);
    }

    ProgramSlot make_program(const uint8_t* vs, uint32_t vs_size, const uint8_t* fs, uint32_t fs_size) {
        ProgramSlot p;
        p.program = bgfx::createProgram(bgfx::createShader(bgfx::makeRef(vs, vs_size)),
                                        bgfx::createShader(bgfx::makeRef(fs, fs_size)), true);
        return p;
    }

    void native_handles(void*& nwh, void*& ndt, bgfx::NativeWindowHandleType::Enum& type) {
#ifdef YATAIDON_PLATFORM_PS5
        nwh = window;
        ndt = nullptr;
        type = bgfx::NativeWindowHandleType::Default;
#else
        SDL_PropertiesID props = SDL_GetWindowProperties(window);
        if (std::strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
            ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
            nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
            type = bgfx::NativeWindowHandleType::Wayland;
        } else {
            ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
            nwh = (void*)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
            type = bgfx::NativeWindowHandleType::Default;
        }
#endif
    }

    bgfx::RendererType::Enum renderer_from_env() {
        const char* name = std::getenv("YATAIDON_BGFX_RENDERER");
        if (!name) {
            return bgfx::RendererType::Vulkan; // only SPIR-V shaders are built so far
        }
        for (int i = 0; i < bgfx::RendererType::Count; i++) {
            if (SDL_strcasecmp(name, bgfx::getRendererName((bgfx::RendererType::Enum)i)) == 0) {
                return (bgfx::RendererType::Enum)i;
            }
        }
        spdlog::warn("bgfx: unknown YATAIDON_BGFX_RENDERER '{}', using Vulkan", name);
        return bgfx::RendererType::Vulkan;
    }

    void reset_backbuffer() {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        bgfx::SwapChain sc;
        bgfx::NativeWindowHandleType::Enum type;
        native_handles(sc.nwh, sc.ndt, type);
        sc.width = (uint32_t)w;
        sc.height = (uint32_t)h;
        bgfx::reset(reset_flags, &sc);
    }

    // Logs bgfx errors and saves requested screenshots as PNG (RGBA, top row first).
    struct Callback : bgfx::CallbackI {
        void fatal(const char* file, uint16_t line, bgfx::Fatal::Enum, const char* str) override {
            spdlog::critical("bgfx fatal {}:{}: {}", file, line, str);
            std::abort();
        }
        void traceVargs(const char*, uint16_t, const char*, va_list) override {}
        void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
        void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
        void profilerEnd() override {}
        // Vulkan pipeline caches, so pipelines compile once per machine instead of on every run.
        static std::string cache_path(uint64_t id) { return "cache/bgfx/" + std::to_string(id) + ".bin"; }
        uint32_t cacheReadSize(uint64_t id) override {
            std::error_code ec;
            const auto size = std::filesystem::file_size(cache_path(id), ec);
            return ec ? 0 : (uint32_t)size;
        }
        bool cacheRead(uint64_t id, void* data, uint32_t size) override {
            std::ifstream f(cache_path(id), std::ios::binary);
            return f.read(static_cast<char*>(data), size) && f.gcount() == size;
        }
        void cacheWrite(uint64_t id, const void* data, uint32_t size) override {
            std::error_code ec;
            std::filesystem::create_directories("cache/bgfx", ec);
            std::ofstream(cache_path(id), std::ios::binary).write(static_cast<const char*>(data), size);
        }
        void screenShot(const char* path, uint32_t w, uint32_t h, uint32_t pitch, bgfx::TextureFormat::Enum format,
                        const void* data, uint32_t, bool yflip) override {
            Image img = gen_image_color((int)w, (int)h, Color::BLACK);
            auto* out = static_cast<uint8_t*>(img.data);
            const bool bgra = format == bgfx::TextureFormat::BGRA8;
            for (uint32_t y = 0; y < h; y++) {
                const auto* row = static_cast<const uint8_t*>(data) + (yflip ? h - 1 - y : y) * pitch;
                for (uint32_t x = 0; x < w; x++) {
                    uint8_t* o = out + (y * w + x) * 4;
                    o[0] = row[x * 4 + (bgra ? 2 : 0)];
                    o[1] = row[x * 4 + 1];
                    o[2] = row[x * 4 + (bgra ? 0 : 2)];
                    o[3] = 255;
                }
            }
            export_image(img, path);
            unload_image(img);
            spdlog::info("bgfx: saved {}", path);
        }
        void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
        void captureEnd() override {}
        void captureFrame(const void*, uint32_t) override {}
    } callback;

    // Debug aid: YATAIDON_BGFX_SHOT="3,8:/tmp/shot" saves /tmp/shot0.png at 3 s, /tmp/shot1.png at 8 s.
    std::vector<double> shot_times;
    std::string shot_prefix;
    size_t shot_index = 0;

    void parse_shot_env() {
        const char* env = std::getenv("YATAIDON_BGFX_SHOT");
        const char* colon = env ? std::strchr(env, ':') : nullptr;
        if (!colon) {
            return;
        }
        shot_prefix = colon + 1;
        for (const char* p = env; p < colon; p = std::strchr(p, ',') ? std::strchr(p, ',') + 1 : colon) {
            shot_times.push_back(std::atof(p));
        }
    }

    void maybe_request_shot() {
        if (shot_index < shot_times.size() && SDL_GetTicks() / 1000.0 >= shot_times[shot_index]) {
            const std::string path = shot_prefix + std::to_string(shot_index++) + ".png";
            bgfx::requestScreenShot(BGFX_INVALID_HANDLE, path.c_str());
        }
    }

    // affine 2D camera as a bgfx (column-major) matrix, matching raylib's GetCameraMatrix2D
    std::array<float, 16> camera_matrix(const Camera2D& cam) {
        const float s = sinf(cam.rotation * deg2rad) * cam.zoom, c = cosf(cam.rotation * deg2rad) * cam.zoom;
        const float tx = cam.offset.x - (c * cam.target.x - s * cam.target.y);
        const float ty = cam.offset.y - (s * cam.target.x + c * cam.target.y);
        return {c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, tx, ty, 0, 1};
    }

} // namespace

// ---- window ---------------------------------------------------------------------------

SDL_Window* main_window() {
    return window;
}

void set_window_flags(unsigned flags) {
    window_flags = flags;
}

void quiet_raylib_log() {}

void disable_exit_key() {}

void init_window(int width, int height, const char* title) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        spdlog::critical("SDL video init failed: {}", SDL_GetError());
        std::abort();
    }
#ifdef YATAIDON_USE_SDL2
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
                              (window_flags & window_flag::RESIZABLE) ? SDL_WINDOW_RESIZABLE : 0);
#else
    window = SDL_CreateWindow(title, width, height, (window_flags & window_flag::RESIZABLE) ? SDL_WINDOW_RESIZABLE : 0);
#endif
    if (!window) {
        spdlog::critical("SDL_CreateWindow failed: {}", SDL_GetError());
        std::abort();
    }
    bgfx::renderFrame(); // before init: bgfx renders on this thread, no render thread

    bgfx::Init init;
    init.type = renderer_from_env();
    native_handles(init.swapChain.nwh, init.swapChain.ndt, init.platformData.type);
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    init.swapChain.width = (uint32_t)w;
    init.swapChain.height = (uint32_t)h;
    reset_flags = (window_flags & window_flag::VSYNC) ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
#ifdef YATAIDON_PLATFORM_PS5
    reset_flags |= BGFX_RESET_VSYNC;
#endif
    init.reset = reset_flags;
    init.callback = &callback;
    parse_shot_env();
    if (!bgfx::init(init)) {
        spdlog::critical("bgfx init failed");
        std::abort();
    }
    caps = bgfx::getCaps();
    spdlog::info("bgfx: {} renderer, {}x{}", bgfx::getRendererName(bgfx::getRendererType()), w, h);
    if (const char* dbg = std::getenv("YATAIDON_BGFX_DEBUG")) {
        bgfx::setDebug(std::atoi(dbg) ? BGFX_DEBUG_STATS : BGFX_DEBUG_NONE);
    }

    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    s_tex_color = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
    programs.push_back(make_program(vs_2d_spv, sizeof(vs_2d_spv), fs_2d_spv, sizeof(fs_2d_spv)));

    Image white = gen_image_color(1, 1, Color::WHITE);
    white_texture = upload_texture(white, false);
    unload_image(white);

    default_font = load_font_from_memory(default_font_ttf(), 32);

    set_separate_alpha_blend();
    cur_blend = blend_state(BLEND_ALPHA);
    cur_model = identity;
}

void close_window() {
    verts.clear();
    for (auto& t : targets) {
        if (bgfx::isValid(t.fb)) {
            bgfx::destroy(t.fb);
        }
    }
    for (auto& t : textures) {
        if (bgfx::isValid(t.handle)) {
            bgfx::destroy(t.handle);
        }
    }
    for (auto& p : programs) {
        if (bgfx::isValid(p.program)) {
            bgfx::destroy(p.program);
            for (auto u : p.uniforms) {
                bgfx::destroy(u);
            }
        }
    }
    targets.clear();
    textures.clear();
    programs.clear();
    bgfx::destroy(s_tex_color);
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    window = nullptr;
}

void poll_window_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) { // the game's input reads events through SDL event watches
        if (e.type == SDL_EVENT_QUIT) {
            should_close = true;
#ifdef YATAIDON_USE_SDL2
        } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
#else
        } else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
#endif
            reset_backbuffer();
        }
    }
}

bool window_should_close() {
    return should_close;
}

void toggle_fullscreen() {
    SDL_SetWindowFullscreen(window, !is_window_fullscreen());
}

void toggle_borderless_windowed() {
    toggle_fullscreen(); // SDL3 fullscreen without a display mode is borderless desktop, as raylib's SDL path
}

bool is_window_fullscreen() {
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
}

void set_window_size(int width, int height) {
    SDL_SetWindowSize(window, width, height);
}

int get_screen_width() {
    int w = 0;
    SDL_GetWindowSize(window, &w, nullptr);
    return w;
}

int get_screen_height() {
    int h = 0;
    SDL_GetWindowSize(window, nullptr, &h);
    return h;
}

int get_render_width() {
    if (pass.target) { // like raylib: the render target's size while one is bound
        return pass.width;
    }
    int w = 0, h = 0;
    window_pixels(w, h);
    return w;
}

int get_render_height() {
    if (pass.target) {
        return pass.height;
    }
    int w = 0, h = 0;
    window_pixels(w, h);
    return h;
}

int get_monitor_refresh_rate() {
#ifdef YATAIDON_USE_SDL2
    SDL_DisplayMode storage{};
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &storage) == 0 ? &storage : nullptr;
#else
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
#endif
    return mode ? (int)mode->refresh_rate : 0;
}

// ---- frame / passes -------------------------------------------------------------------

void begin_drawing() {
    open_pass(0);
    cur_model = identity;
}

void end_drawing() {
    flush();
}

void swap_screen_buffer() {
    flush();
    if (!screen_drawn) { // the backbuffer still needs a pass to be presented
        if (pass.target != 0) {
            open_pass(0);
        }
        bgfx::touch(pass.view);
    }
    screen_drawn = false;
    pending_upload_bytes = 0; // uploaded by this frame
    submits_this_frame = 0;
    maybe_request_shot();
    bgfx::frame();
    frame_open = false;
    pass.target = 0;
}

void clear_background(Color color) {
    open_pass(pass.target, true, color);
}

RenderTarget load_render_texture(int width, int height) {
    TargetSlot t;
    t.width = width;
    t.height = height;
    TextureSlot color;
    color.handle =
        bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
    bgfx::TextureHandle depth = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1,
                                                      bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT_WRITE_ONLY);
    bgfx::Attachment att[2];
    att[0].init(color.handle, bgfx::Access::Write, 0, 1, 0, BGFX_ATTACHMENT_NONE);
    att[1].init(depth, bgfx::Access::Write, 0, 1, 0, BGFX_ATTACHMENT_NONE);
    t.fb = bgfx::createFrameBuffer(2, att, false);
    bgfx::destroy(depth); // the framebuffer keeps it alive
    t.color = add_slot(color);

    RenderTarget rt;
    rt.id = add_slot(t);
    rt.texture.id = t.color;
    rt.texture.width = width;
    rt.texture.height = height;
    rt.texture.mipmaps = 1;
    rt.texture.format = pixel_format::RGBA8;
    return rt;
}

void unload_render_texture(RenderTarget target) {
    if (!target.id || target.id > targets.size() || !bgfx::isValid(targets[target.id - 1].fb)) {
        return;
    }
    TargetSlot& t = targets[target.id - 1];
    bgfx::destroy(t.fb);
    t.fb = BGFX_INVALID_HANDLE;
    Texture color;
    color.id = t.color;
    unload_texture(color);
}

void begin_texture_mode(RenderTarget target) {
    open_pass(target.id);
    cur_model = identity;
}

void end_texture_mode() {
    open_pass(0);
    cur_model = identity;
}

// ---- state ----------------------------------------------------------------------------

void begin_scissor_mode(int x, int y, int width, int height) {
    if (pass.flip) {
        y = pass.height - (y + height);
    }
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    Scissor s{true, (uint16_t)x, (uint16_t)y, (uint16_t)std::max(width, 0), (uint16_t)std::max(height, 0)};
    if (!(s == cur_scissor)) {
        flush();
        cur_scissor = s;
    }
}

void end_scissor_mode() {
    if (cur_scissor.on) {
        flush();
        cur_scissor = {};
    }
}

void begin_blend_mode(int mode) {
    const uint64_t b = blend_state(mode);
    if (b != cur_blend) {
        flush();
        cur_blend = b;
    }
}

void end_blend_mode() {
    begin_blend_mode(BLEND_ALPHA);
}

void set_separate_alpha_blend() {
    custom_separate_blend = BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA,
                                                           BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
}

void begin_mode_2d(Camera2D camera) {
    flush();
    cur_model = camera_matrix(camera);
}

void end_mode_2d() {
    flush();
    cur_model = identity;
}

Vec2 get_world_to_screen_2d(Vec2 p, Camera2D camera) {
    const auto m = camera_matrix(camera);
    return {m[0] * p.x + m[4] * p.y + m[12], m[1] * p.x + m[5] * p.y + m[13]};
}

Vec2 get_screen_to_world_2d(Vec2 p, Camera2D camera) {
    const auto m = camera_matrix(camera);
    const float det = m[0] * m[5] - m[4] * m[1];
    if (det == 0.0f) {
        return {};
    }
    const float x = p.x - m[12], y = p.y - m[13];
    return {(m[5] * x - m[4] * y) / det, (m[0] * y - m[1] * x) / det};
}

// ---- textures -------------------------------------------------------------------------

Texture load_texture_from_image(Image image, bool mipmaps) {
    Texture tex;
    if (!image.data || image.width <= 0 || image.height <= 0) {
        return tex;
    }
    tex.id = upload_texture(image, mipmaps);
    tex.width = image.width;
    tex.height = image.height;
    tex.mipmaps = 1;
    tex.format = image.format;
    if (mipmaps) {
        for (int w = image.width, h = image.height; w > 1 || h > 1; w /= 2, h /= 2) {
            tex.mipmaps++;
        }
    }
    return tex;
}

void unload_texture(Texture tex) {
    if (TextureSlot* t = texture_slot(tex.id)) {
        if (cur_texture == tex.id) {
            flush();
        }
        bgfx::destroy(t->handle);
        t->handle = BGFX_INVALID_HANDLE;
    }
}

bool is_texture_valid(Texture tex) {
    return texture_slot(tex.id) != nullptr;
}

void set_texture_filter(Texture tex, int filter) {
    if (TextureSlot* t = texture_slot(tex.id)) {
        if (cur_texture == tex.id) {
            flush();
        }
        t->filter = filter;
    }
}

void set_texture_wrap(Texture tex, int wrap) {
    if (TextureSlot* t = texture_slot(tex.id)) {
        if (cur_texture == tex.id) {
            flush();
        }
        t->wrap = wrap;
    }
}

void update_texture(Texture tex, const void* pixels) {
    TextureSlot* t = texture_slot(tex.id);
    if (!t || !pixels) {
        return;
    }
    if (cur_texture == tex.id) {
        flush();
    }
    Image src{const_cast<void*>(pixels), tex.width, tex.height, 1, tex.format};
    Image rgba = src;
    if (tex.format != pixel_format::RGBA8) {
        rgba = image_copy(src);
        image_format(&rgba, pixel_format::RGBA8);
    }
    bgfx::updateTexture2D(t->handle, 0, 0, 0, 0, (uint16_t)tex.width, (uint16_t)tex.height,
                          bgfx::copy(rgba.data, (uint32_t)(tex.width * tex.height * 4)));
    if (rgba.data != pixels) {
        unload_image(rgba);
    }
}

Image load_image_from_texture(Texture) {
    // ponytail: GPU readback (blit + readTexture, 2 frames late) not implemented; only chara_3d recolor uses it
    spdlog::warn("bgfx: load_image_from_texture not implemented yet");
    return {};
}

Image load_image_from_screen() {
    // ponytail: needs bgfx::requestScreenShot + a CallbackI; F12 screenshots are disabled until then
    spdlog::warn("bgfx: screenshots not implemented yet");
    return {};
}

// ---- shaders --------------------------------------------------------------------------

Shader load_shader(const char* vs_path, const char* fs_path) {
    (void)vs_path; // every 2D shader pairs with the default vertex stage (dummy.vs)
    const std::string fs = fs_path ? fs_path : "";
    Shader shader;
    if (fs.ends_with("colortransform.fs")) {
        shader.id = add_program(
            make_program(vs_2d_spv, sizeof(vs_2d_spv), fs_colortransform_spv, sizeof(fs_colortransform_spv)));
    } else {
        static bool warned = false;
        if (!warned) {
            spdlog::warn("bgfx: shader '{}' not ported yet (3D shaders come with model support)", fs);
            warned = true;
        }
    }
    return shader;
}

void unload_shader(Shader shader) {
    if (!is_shader_valid(shader)) {
        return;
    }
    if (cur_program == shader.id) {
        flush();
        cur_program = 0;
    }
    ProgramSlot& p = programs[shader.id];
    bgfx::destroy(p.program);
    for (auto u : p.uniforms) {
        bgfx::destroy(u);
    }
    p = ProgramSlot{};
}

bool is_shader_valid(Shader shader) {
    return shader.id && shader.id < programs.size() && bgfx::isValid(programs[shader.id].program);
}

int get_shader_location(Shader shader, const char* name) {
    if (!is_shader_valid(shader)) {
        return -1;
    }
    ProgramSlot& p = programs[shader.id];
    for (size_t i = 0; i < p.names.size(); i++) {
        if (p.names[i] == name) {
            return (int)i;
        }
    }
    p.names.emplace_back(name);
    p.uniforms.push_back(bgfx::createUniform(name, bgfx::UniformType::Vec4));
    p.values.push_back({});
    return (int)p.names.size() - 1;
}

void set_shader_value(Shader shader, int loc, const void* value, int uniform_type) {
    if (!is_shader_valid(shader) || loc < 0 || (size_t)loc >= programs[shader.id].values.size()) {
        return;
    }
    if (cur_program == shader.id) {
        flush(); // uniforms apply per submit
    }
    const int n = uniform_type == shader_uniform::VEC3 ? 3 : uniform_type == shader_uniform::VEC2 ? 2 : 1;
    auto& v = programs[shader.id].values[loc];
    v = {};
    std::memcpy(v.data(), value, n * sizeof(float));
}

void begin_shader_mode(Shader shader) {
    const unsigned prog = is_shader_valid(shader) ? shader.id : 0;
    if (prog != cur_program) {
        flush();
        cur_program = prog;
    }
}

void end_shader_mode() {
    begin_shader_mode(Shader{});
}

// ---- drawing --------------------------------------------------------------------------

void draw_texture_pro(Texture tex, Rect src, Rect dst, Vec2 origin, float rotation, Color tint) {
    if (!texture_slot(tex.id)) {
        return;
    }
    const float w = (float)tex.width, h = (float)tex.height;
    bool flip_x = false;
    if (src.width < 0) {
        flip_x = true;
        src.width *= -1;
    }
    if (src.height < 0) {
        src.y -= src.height;
    }
    dst.width = std::abs(dst.width);
    dst.height = std::abs(dst.height);

    Vec2 tl, tr, bl, br;
    if (rotation == 0.0f) {
        const float x = dst.x - origin.x, y = dst.y - origin.y;
        tl = {x, y};
        tr = {x + dst.width, y};
        bl = {x, y + dst.height};
        br = {x + dst.width, y + dst.height};
    } else {
        const float s = sinf(rotation * deg2rad), c = cosf(rotation * deg2rad);
        const float dx = -origin.x, dy = -origin.y;
        tl = {dst.x + dx * c - dy * s, dst.y + dx * s + dy * c};
        tr = {dst.x + (dx + dst.width) * c - dy * s, dst.y + (dx + dst.width) * s + dy * c};
        bl = {dst.x + dx * c - (dy + dst.height) * s, dst.y + dx * s + (dy + dst.height) * c};
        br = {dst.x + (dx + dst.width) * c - (dy + dst.height) * s,
              dst.y + (dx + dst.width) * s + (dy + dst.height) * c};
    }
    const float u0 = src.x / w, u1 = (src.x + src.width) / w;
    const float v0 = src.y / h, v1 = (src.y + src.height) / h;
    use(tex.id, false);
    if (flip_x) {
        push_quad(tl, bl, br, tr, {u1, v0}, {u1, v1}, {u0, v1}, {u0, v0}, tint);
    } else {
        push_quad(tl, bl, br, tr, {u0, v0}, {u0, v1}, {u1, v1}, {u1, v0}, tint);
    }
}

void draw_rectangle(Rect rec, Color color) {
    fill_rect(rec, color);
}

void draw_rectangle(int x, int y, int w, int h, Color color) {
    fill_rect({(float)x, (float)y, (float)w, (float)h}, color);
}

void draw_rectangle_lines_ex(Rect rec, float thick, Color color) {
    if (thick > rec.width || thick > rec.height) {
        thick = rec.width >= rec.height ? rec.height / 2 : rec.width / 2;
    }
    fill_rect({rec.x, rec.y, rec.width, thick}, color);
    fill_rect({rec.x, rec.y - thick + rec.height, rec.width, thick}, color);
    fill_rect({rec.x, rec.y + thick, thick, rec.height - thick * 2.0f}, color);
    fill_rect({rec.x - thick + rec.width, rec.y + thick, thick, rec.height - thick * 2.0f}, color);
}

void draw_rectangle_lines(int x, int y, int w, int h, Color color) {
    const float ox = 0.5f / cur_model[0], oy = 0.5f / cur_model[5]; // pixel centers, as raylib
    const float l = (float)x + ox, t = (float)y + oy, r = (float)(x + w) - ox, b = (float)(y + h) - oy;
    push_line({l, t}, {r, t}, color);
    push_line({r, t}, {r, b}, color);
    push_line({r, b}, {l, b}, color);
    push_line({l, b}, {l, t}, color);
}

void draw_line(int x0, int y0, int x1, int y1, Color color) {
    push_line({(float)x0, (float)y0}, {(float)x1, (float)y1}, color);
}

void draw_rectangle_rounded(Rect rec, float roundness, int segments, Color color) {
    if (roundness <= 0.0f) {
        fill_rect(rec, color);
        return;
    }
    roundness = std::min(roundness, 1.0f);
    const float radius = (rec.width > rec.height ? rec.height : rec.width) * roundness / 2;
    if (radius <= 0.0f) {
        return;
    }
    segments = corner_segments(segments, radius);
    const float x0 = rec.x + radius, x1 = rec.x + rec.width - radius;
    const float y0 = rec.y + radius, y1 = rec.y + rec.height - radius;
    use(white_texture, false);
    corner({x0, y0}, 0, radius, 180, segments, color);
    corner({x1, y0}, 0, radius, 270, segments, color);
    corner({x1, y1}, 0, radius, 0, segments, color);
    corner({x0, y1}, 0, radius, 90, segments, color);
    fill_rect({x0, rec.y, x1 - x0, rec.height}, color);
    fill_rect({rec.x, y0, radius, y1 - y0}, color);
    fill_rect({x1, y0, radius, y1 - y0}, color);
}

void draw_rectangle_rounded_lines_ex(Rect rec, float roundness, int segments, float thick, Color color) {
    if (roundness <= 0.0f) {
        draw_rectangle_lines_ex(rec, thick, color);
        return;
    }
    roundness = std::min(roundness, 1.0f);
    const float radius = (rec.width > rec.height ? rec.height : rec.width) * roundness / 2;
    if (radius <= 0.0f) {
        return;
    }
    // ponytail: inward outlines only (thick >= 0), the only kind the game draws
    const float outer = radius;
    float inner = outer - thick, band = thick;
    if (inner <= 0.0f) {
        inner = 0.0f;
        band = outer;
        draw_rectangle_lines_ex({rec.x + outer, rec.y + outer, rec.width - outer * 2, rec.height - outer * 2},
                                thick - outer, color);
    }
    segments = corner_segments(segments, outer);
    const float x0 = rec.x + outer, x1 = rec.x + rec.width - outer;
    const float y0 = rec.y + outer, y1 = rec.y + rec.height - outer;
    use(white_texture, false);
    corner({x0, y0}, inner, outer, 180, segments, color);
    corner({x1, y0}, inner, outer, 270, segments, color);
    corner({x1, y1}, inner, outer, 0, segments, color);
    corner({x0, y1}, inner, outer, 90, segments, color);
    fill_rect({x0, rec.y, x1 - x0, band}, color);
    fill_rect({x0, rec.y + rec.height - band, x1 - x0, band}, color);
    fill_rect({rec.x, y0, band, y1 - y0}, color);
    fill_rect({rec.x + rec.width - band, y0, band, y1 - y0}, color);
}

void draw_text_ex(Font font, const char* text, Vec2 pos, float font_size, float spacing, Color tint) {
    if (font.texture.id == 0) {
        font = default_font;
    }
    if (!text || !font.glyphs || !font.recs) {
        return;
    }
    const float scale = font_size / (float)font.baseSize;
    const float pad = (float)font.glyphPadding;
    float ox = 0.0f, oy = 0.0f;
    for (const char* p = text; *p;) {
        int n = 0;
        const int cp = get_codepoint_next(p, &n);
        p += n;
        const int gi = get_glyph_index(font, cp);
        const GlyphInfo& g = font.glyphs[gi];
        const Rect& r = font.recs[gi];
        if (cp == '\n') {
            oy += font_size + TEXT_LINE_SPACING;
            ox = 0.0f;
            continue;
        }
        if (cp != ' ' && cp != '\t') {
            draw_texture_pro(font.texture, {r.x - pad, r.y - pad, r.width + 2 * pad, r.height + 2 * pad},
                             {pos.x + ox + g.offsetX * scale - pad * scale,
                              pos.y + oy + g.offsetY * scale - pad * scale, (r.width + 2 * pad) * scale,
                              (r.height + 2 * pad) * scale},
                             {0, 0}, 0.0f, tint);
        }
        ox += g.advanceX == 0 ? r.width * scale + spacing : (float)g.advanceX * scale + spacing;
    }
}

void draw_text(const char* text, int x, int y, int font_size, Color color) {
    font_size = std::max(font_size, 10); // raylib's default-font minimum
    draw_text_ex(default_font, text, {(float)x, (float)y}, (float)font_size, (float)(font_size / 10), color);
}

int measure_text(const char* text, int font_size) {
    font_size = std::max(font_size, 10);
    return (int)measure_text_ex(default_font, text, (float)font_size, (float)(font_size / 10)).x;
}

Font get_font_default() {
    return default_font;
}

// ---- 3D (not ported yet) -----------------

Model load_model(const char* path) {
    static bool warned = false;
    if (!warned) {
        spdlog::warn("bgfx: 3D models are not supported yet ({})", path);
        warned = true;
    }
    Model model;
    model.transform = matrix_scale(1, 1, 1);
    return model;
}

void unload_model(Model) {}

ModelAnimation* load_model_animations(const char*, int* count) {
    *count = 0;
    return nullptr;
}

void unload_model_animations(ModelAnimation*, int) {}
void update_model_animation(Model, ModelAnimation, float) {}
void draw_model(Model, Vec3, float, Color) {}
void draw_mesh(Mesh, Material, Mat4) {}
void update_mesh_buffer(Mesh, int, const void*, int, int) {}
void set_material_texture(Material*, int, Texture) {}
void begin_mode_3d(Camera3D) {
    flush();
}
void end_mode_3d() {
    flush();
}
void set_backface_culling(bool) {}
void set_cull_face(int) {}
