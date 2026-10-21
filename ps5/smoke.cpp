// Build/link probe for YataiDON's raylib + SDL2 route on PS5.
// Native title assembly must provide the SDK's heap and runtime integration.
#include "../src/libs/sdl_compat.h"
#include <raylib.h>
#include <cstdio>

int main() {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) < 0) {
        std::fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_DisplayMode display{};
    if (SDL_GetDesktopDisplayMode(0, &display) < 0) {
        std::fprintf(stderr, "Display query failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(display.w, display.h, "YataiDON PS5 graphics probe");
    if (!IsWindowReady()) {
        std::fprintf(stderr, "raylib window failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    RenderTexture2D canvas = LoadRenderTexture(640, 360);
    if (!IsRenderTextureValid(canvas)) {
        CloseWindow();
        return 1;
    }
    unsigned frames = 0;
    for (; frames < 600 && !WindowShouldClose(); ++frames) {
        BeginTextureMode(canvas);
        ClearBackground(DARKBLUE);
        DrawCircle(320, 180, 64, RED);
        DrawText("YataiDON / PS5 / raylib / SDL2", 20, 20, 24, WHITE);
        DrawText(IsGamepadAvailable(0) ? "Controller connected" : "Waiting for controller", 20, 310, 20, WHITE);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.texture, {0, 0, 640, -360},
                       {0, 0, static_cast<float>(display.w), static_cast<float>(display.h)},
                       {0, 0}, 0, WHITE);
        EndDrawing();
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) break;
    }
    std::printf("[yataidon-ps5] rendered=%u controller=%d\n", frames, IsGamepadAvailable(0));
    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}
