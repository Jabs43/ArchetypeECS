#include "raylib.h"
#include "Render.h"
#include "TankScene.h"
#include <algorithm>
#include <exception>
#include <memory>

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Archetype ECS - Tank and collidable boxes");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(320, 240);
    SetTargetFPS(144);
    int result = 0;
    try {
        // This scope releases the render texture before CloseWindow().
        Render render(Demo::Width, Demo::Height);
        auto scene = std::make_unique<Demo::TankScene>();
        float accumulator = 0;
        while (!WindowShouldClose()) {
            render.UpdateMouseMapping();
            if (IsKeyPressed(KEY_R)) {
                scene = std::make_unique<Demo::TankScene>();
                accumulator = 0;
            }
            const Demo::Input input{
                float(IsKeyDown(KEY_W))-float(IsKeyDown(KEY_S)),
                float(IsKeyDown(KEY_D))-float(IsKeyDown(KEY_A)),
                render.CalcVirtualMouse(),
                render.IsMouseInsideGame() && IsMouseButtonDown(MOUSE_BUTTON_LEFT)
            };
            accumulator += std::min(GetFrameTime(), 0.1f);
            while (accumulator >= Demo::FixedStep) {
                scene->Update(input, Demo::FixedStep);
                accumulator -= Demo::FixedStep;
            }
            render.Begin();
            ClearBackground(BLACK);
            scene->Draw();
            render.End();
            render.DrawRenderTexture();
        }
    } catch (const std::exception& error) {
        TraceLog(LOG_ERROR, "%s", error.what());
        result = 1;
    }
    CloseWindow();
    return result;
}
