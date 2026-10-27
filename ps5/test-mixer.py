"""Compile the actual sound mixer/control code with host device/codec dependencies removed."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/libs/audio/audio.cpp').read_text()
header = (root / 'src/libs/audio/audio.h').read_text()
sound = header[header.index('struct sound {'):header.index('\nstruct music {')]
enum = header[header.index('enum class VolumePreset'):header.index('\nstruct VirtualFile')]
mix = source[source.index('void AudioEngine::mix('):source.index('    for (auto& [name, mus] : engine->music_streams)')] + '}\n'
play = source[source.index('static float get_preset_volume('):source.index('\nbool AudioEngine::has_sound(')]
prefix = r'''
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <thread>
#define PROFILE_SCOPE()
struct SRC_STATE;
namespace yd { template<class T> using atomic_ref = std::atomic_ref<T>; }
namespace spdlog { template<class... T> void warn(const char*, T...) {} }
'''
engine = r'''
struct VolumeConfig { float sound=1, music=1, voice=1, hitsound=1, attract_mode=1; };
class AudioEngine {
public:
    std::shared_mutex rw_lock;
    std::mutex play_count_lock;
    std::unordered_map<std::string, uint64_t> play_counts;
    std::unordered_map<std::string, sound> sounds;
    VolumeConfig volume_presets;
    static void mix(float*, unsigned, AudioEngine*);
    void play_sound(const std::string&, VolumePreset = VolumePreset::NONE, float = 1);
};
'''
tests = r'''
int main() {
    AudioEngine engine;
    float samples[] = {0.2f, 0.4f, 0.6f};
    auto& snd = engine.sounds["balloon_pop"];
    snd.data=samples; snd.channels=1; snd.frame_count=3; snd.loop=true;
    engine.play_sound("balloon_pop");
    float out[16];
    AudioEngine::mix(out, 8, &engine);
    for (int i=0; i<8; ++i) assert(out[i*2] == samples[i%3]*0.5f);
    assert(snd.is_playing);
    snd.loop=false;
    engine.play_sound("balloon_pop");
    AudioEngine::mix(out, 8, &engine);
    assert(!snd.is_playing && out[6] == 0);
    // A trigger at EOF must wait for the in-flight mix buffer's writeback.
    std::shared_lock in_flight(engine.rw_lock);
    auto trigger = std::async(std::launch::async, [&] { engine.play_sound("balloon_pop"); });
    assert(trigger.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout);
    in_flight.unlock(); trigger.get();
    assert(snd.current_frame == 0 && snd.is_playing);
    AudioEngine::mix(out, 1, &engine);
    assert(out[0] == samples[0]*0.5f);
    snd.normalization_gain = 0.5f;
    engine.play_sound("balloon_pop");
    AudioEngine::mix(out, 1, &engine);
    assert(out[0] == samples[0]*0.25f);
}
'''
with tempfile.TemporaryDirectory(prefix='yataidon-mixer-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(prefix + enum + sound + engine + mix + play + tests)
    subprocess.run(['c++', '-std=c++20', '-pthread', '-fsanitize=address,undefined', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: actual mixer loops, one-shots, retrigger serialization and normalization')
