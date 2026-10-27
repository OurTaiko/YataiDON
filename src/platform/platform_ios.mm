#include "platform_ios.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <stdexcept>

// Only code that calls Objective-C APIs lives here; see platform_ios.cpp.
// Do not include gfx.h: Foundation's MacTypes.h defines a conflicting Rect.

void ios_request_audio_buffer() {
    @autoreleasepool {
        NSError* error = nil;
        if (![[AVAudioSession sharedInstance] setPreferredIOBufferDuration:0.005 error:&error]) {
            spdlog::warn("iOS latency: buffer preference rejected: {}", error.localizedDescription.UTF8String);
        }
    }
}

void ios_prepare_filesystem() {
    namespace fs = std::filesystem;
    @autoreleasepool {
        NSURL* documents = [[[NSFileManager defaultManager]
            URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] firstObject];
        if (!documents) throw std::runtime_error("Cannot locate iOS Documents directory");
        fs::path destination(documents.fileSystemRepresentation);
        fs::path resources([NSBundle mainBundle].resourcePath.fileSystemRepresentation);
        resources /= "GameData";
        fs::create_directories(destination);

        // Keep the existing relative-path asset loaders and all writable files
        // together. Copy only missing files so upgrades preserve user content.
        std::error_code ec;
        if (!fs::is_directory(resources, ec)) {
            spdlog::warn("iOS: bundled GameData missing or not a directory: {}", resources.string());
        } else {
            for (const auto& entry : fs::recursive_directory_iterator(resources, ec)) {
                // The iterator already yields paths rooted at resources. Avoid
                // canonicalizing both paths (and querying every ancestor) per file.
                fs::path relative = entry.path().lexically_relative(resources);
                fs::path target = destination / relative;
                if (entry.is_directory(ec)) {
                    fs::create_directories(target, ec);
                } else if (entry.is_regular_file(ec)) {
                    // Parent directories were visited before their children.
                    // Shaders ship with the executable and must match its version.
                    bool shader = *relative.begin() == "shader";
                    if (shader || !fs::exists(target, ec)) {
                        fs::copy_file(entry.path(), target, shader ? fs::copy_options::overwrite_existing
                                                                  : fs::copy_options::skip_existing, ec);
                        if (ec) {
                            spdlog::warn("iOS: failed to copy {}: {}", entry.path().string(), ec.message());
                            ec.clear();
                        }
                    }
                }
            }
        }
        fs::create_directories(destination / "Songs");
        fs::current_path(destination);
    }
}
