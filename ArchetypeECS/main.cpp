#include "raylib.h"
#include "Render.h"
#include "TankScene.h"
#include <algorithm>
#include <exception>
#include <memory>

//#define RUN_BENCHMARK_PHYSICS true

#ifdef RUN_BENCHMARK_PHYSICS
#include "PhysicsBenchmark.h"
#endif

#ifdef RUN_SCENEMANAGER_TEST
#include "SceneManagerTest.h"
#endif

#if !defined(RUN_BENCHMARK_PHYSICS) && !defined(RUN_BENCHMARK_RENDER) \
 && !defined(RUN_BENCHMARK_ECS) && !defined(RUN_BENCHMARK_SYSTEMS) && !defined(RUN_SCENEMANAGER_TEST)// <-- compile the game's main ONLY when NOT benchmarking

#pragma message(">>> GAME main compiling")

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Archetype ECS - Tank and collidable boxes");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(320, 240);
    //SetTargetFPS(144);

    int result = 0;
    try {

        // This scope releases the render texture before CloseWindow().
        Render render(Demo::Width, Demo::Height);
        auto scene = std::make_unique<Demo::TankScene>();
        float accumulator = 0;
        while (!WindowShouldClose()) {

            render.UpdateMouseMapping();

#ifdef PHYS_PROFILE
            if (IsKeyPressed(KEY_P)) scene->ReportProfile();
            if (IsKeyPressed(KEY_R)) {
                scene->ResetProfile();
                TraceLog(LOG_INFO, "Physics measurements reset");
            }
#endif
            if (IsKeyPressed(KEY_R)) {//Reset scene
#ifdef PHYS_PROFILE
                scene->ReportProfile();
#endif
                scene = std::make_unique<Demo::TankScene>();
                accumulator = 0;
            }

            const Demo::Input input{
                float(IsKeyDown(KEY_W))-float(IsKeyDown(KEY_S)),
                float(IsKeyDown(KEY_D))-float(IsKeyDown(KEY_A)),
                render.CalcVirtualMouse(),
                render.IsMouseInsideGame() && IsMouseButtonDown(MOUSE_BUTTON_LEFT)
            };

            // --- Fixed step Update ----------------------
            accumulator += std::min(GetFrameTime(), 0.1f);
            while (accumulator >= Demo::FixedStep) {
                scene->Update(input, Demo::FixedStep);
                accumulator -= Demo::FixedStep;
            }

            // --- Draws ----------------------------------
            render.Begin();
            ClearBackground(BLACK);
            scene->Draw();
            render.End();
            render.DrawRenderTexture();
        }

#ifdef PHYS_PROFILE
        scene->ReportProfile();
#endif

    } catch (const std::exception& error) {
        TraceLog(LOG_ERROR, "%s", error.what());
        result = 1;
    }


    CloseWindow();
    return result;
}

#endif // RUN_BENCHMARK