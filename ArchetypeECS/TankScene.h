#pragma once
#include "ECS.h"
#include "PhysicsSystem.h"
#include "raylib.h"
#include <cmath>
#include <random>
#include <vector>

namespace Demo {
inline constexpr int Width = 1920;
inline constexpr int Height = 1080;
inline constexpr int BoxCount = 2000;
inline constexpr float FixedStep = 1.0f / 120.0f;

struct Tank {
    float gunAngle = 0;
    float fireCooldown = 0;
};
struct Bullet { float life = 3; };
struct BoxTag {};
struct Input {
    float drive = 0;
    float turn = 0;
    Vector2 mouse{Width*0.5f, Height*0.5f};
    bool fire = false;
};

class TankScene {
    ECS::ArchetypeRegistry ecs_;
    System::PhysicsSystem physics_{{0, 0, Width, Height}};
    std::vector<ECS::EntityID> dead_;
    struct Shot { Vector2 position, velocity; ECS::EntityID owner; };
    std::vector<Shot> shots_;

    ECS::EntityID AddBody(Vector2 p, Vector2 v, Collider collider, float inverseMass, float restitution) {
        const auto id = ecs_.CreateEntity();
        ecs_.AddComponent<ECS::Position>(id, {p.x, p.y});
        ecs_.AddComponent<ECS::Velocity>(id, {v.x, v.y});
        ecs_.AddComponent<Rotation>(id, {});
        ecs_.AddComponent<Collider>(id, collider);
        ecs_.AddComponent<RigidBody>(id, {inverseMass, restitution});
        ecs_.AddComponent<CollisionState>(id, {});
        return id;
    }

public:
    TankScene() {
        ecs_.RegisterComponent<ECS::Position>();
        ecs_.RegisterComponent<ECS::Velocity>();
        ecs_.RegisterComponent<Rotation>();
        ecs_.RegisterComponent<Collider>();
        ecs_.RegisterComponent<RigidBody>();
        ecs_.RegisterComponent<CollisionState>();
        ecs_.RegisterComponent<Tank>();
        ecs_.RegisterComponent<Bullet>();
        ecs_.RegisterComponent<BoxTag>();
        const auto player = AddBody({Width*0.5f, Height*0.5f}, {}, Collider::Box(100, 60), 0.05f, 0.1f);
        ecs_.AddComponent<Tank>(player, {});

        std::mt19937 random(42);
        std::uniform_real_distribution<float> x(6, Width-6), y(6, Height-6);
        for (int i = 0; i < BoxCount; ++i) {
            Vector2 position;
            do { position = {x(random), y(random)}; }
            while (std::fabs(position.x-Width*0.5f) < 85 && std::fabs(position.y-Height*0.5f) < 65);
            const auto id = AddBody(position, {}, Collider::Box(10, 10), 1, 0.3f);
            ecs_.AddComponent<BoxTag>(id, {});
        }
    }

    ECS::ArchetypeRegistry& Registry() { return ecs_; }

    void Update(const Input& input, float dt) {
        shots_.clear();
        ecs_.ForEach<ECS::Position, ECS::Velocity, Rotation, Tank>(
            [&](ECS::EntityID id, ECS::Position& p, ECS::Velocity& v, Rotation& r, Tank& tank) {
                r.radians = std::remainder(r.radians + input.turn*3.0f*dt, 2*PI);
                v = {std::cos(r.radians)*300*input.drive, std::sin(r.radians)*300*input.drive};
                tank.gunAngle = std::atan2(input.mouse.y-p.y, input.mouse.x-p.x);
                tank.fireCooldown = std::max(0.0f, tank.fireCooldown-dt);
                if (input.fire && tank.fireCooldown <= 0) {
                    tank.fireCooldown = 0.15f;
                    const float c = std::cos(tank.gunAngle), s = std::sin(tank.gunAngle);
                    shots_.push_back({{p.x+c*70, p.y+s*70}, {c*800, s*800}, id});
                }
            });
        // Create only after iteration: archetype migrations can invalidate rows.
        for (const auto& shot : shots_) {
            const auto id = AddBody(shot.position, shot.velocity, Collider::Circle(5), 1, 0);
            ecs_.GetComponent<CollisionState>(id) = {false, true, shot.owner};
            ecs_.AddComponent<Bullet>(id, {});
        }
        physics_.Update(ecs_, dt);
        dead_.clear();
        ecs_.ForEach<Bullet, CollisionState>([&](ECS::EntityID id, Bullet& bullet, CollisionState& state) {
            bullet.life -= dt;
            if (bullet.life <= 0 || state.hit) dead_.push_back(id);
        });
        for (auto id : dead_) ecs_.DestroyEntity(id);
    }

    void Draw() {
        ecs_.ForEach<ECS::Position, Rotation, Collider, CollisionState>(
            [](ECS::EntityID, ECS::Position& p, Rotation& r, Collider& collider, CollisionState& state) {
                const Color color = state.sensor ? YELLOW : (state.hit ? ORANGE : BLUE);
                if (collider.GetShape() == Collider::Shape::Circle) {
                    DrawCircleV({p.x, p.y}, collider.GetCircleRadius(), color);
                } else {
                    const auto h = collider.GetBoxHalfExtents();
                    DrawRectanglePro({p.x, p.y, h.x*2, h.y*2}, h, r.radians*RAD2DEG, color);
                }
            });
        ecs_.ForEach<ECS::Position, Rotation, Collider, Tank>(
            [](ECS::EntityID, ECS::Position& p, Rotation& r, Collider& collider, Tank& tank) {
                const auto h = collider.GetBoxHalfExtents();
                DrawRectanglePro({p.x, p.y, h.x*2, h.y*2}, h, r.radians*RAD2DEG, RED);
                DrawRectanglePro({p.x, p.y, 65, 10}, {0, 5}, tank.gunAngle*RAD2DEG, PURPLE);
            });
        DrawRectangle(10, 10, 900, 78, Fade(BLACK, 0.85f));
        DrawText("W/S drive   A/D turn   Mouse aim   Left click fire   R reset", 20, 20, 24, RAYWHITE);
        DrawText(TextFormat("%d collidable boxes | %zu archetypes", BoxCount, ecs_.ArchetypeCount()), 20, 52, 22, LIGHTGRAY);
        DrawFPS(20, Height-30);
    }
};
} // namespace Demo
