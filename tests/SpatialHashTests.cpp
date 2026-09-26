#include "PhysicsSystem.h"
#include <random>
#include <set>
#include <stdexcept>
#include <iostream>

static void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
static ECS::EntityID add(ECS::ArchetypeRegistry& ecs, float x, float y,
                         Collider shape, bool sensor, float mass = 1, float angle = 0) {
    const auto id = ecs.CreateEntity();
    ecs.AddComponent<ECS::Position>(id, {x,y});
    ecs.AddComponent<ECS::Velocity>(id, {});
    ecs.AddComponent<Rotation>(id, {angle});
    ecs.AddComponent<Collider>(id, shape);
    ecs.AddComponent<RigidBody>(id, {mass,0});
    ecs.AddComponent<CollisionState>(id, {false,sensor});
    return id;
}
static void registerBodies(ECS::ArchetypeRegistry& ecs) {
    ecs.RegisterComponent<ECS::Position>(); ecs.RegisterComponent<ECS::Velocity>();
    ecs.RegisterComponent<Rotation>(); ecs.RegisterComponent<Collider>();
    ecs.RegisterComponent<RigidBody>(); ecs.RegisterComponent<CollisionState>();
}
int main() {
    SpatialHash hash;
    std::vector<GridItem> items;
    std::mt19937 random(42);
    std::uniform_int_distribution<int> coord(-100,100);
    for (unsigned i=0; i<500; ++i)
        items.push_back({i,{float(coord(random)),float(coord(random))},2,2});
    std::vector<GridItem*> pointers, found;
    for (auto& item : items) pointers.push_back(&item);
    hash.build(pointers);
    for (int i=0; i<200; ++i) {
        Rect range{float(coord(random)),float(coord(random)),40,40};
        hash.query(range,found);
        std::set<ECS::EntityID> actual, expected;
        for (auto* item : found) actual.insert(item->id);
        for (auto& item : items) if (range.contains(item.position)) expected.insert(item.id);
        check(actual == expected, "hash differs from brute force");
        check(actual.size() == found.size(), "duplicate query result");
    }
    // Inclusive boundaries and repeated queries must replace, not append.
    hash.query({items[0].position.x,items[0].position.y,0,0},found);
    check(!found.empty(), "point query missed boundary");
    hash.clear(); hash.query({-100,-100,200,200},found);
    check(found.empty(), "clear left stale results");
    pointers.clear(); hash.build(pointers); hash.query({0,0,10,10},found);
    check(found.empty(), "empty build left stale results");

    ECS::ArchetypeRegistry ecs; registerBodies(ecs);
    System::PhysicsSystem physics({-200,-200,400,400});
    physics.Update(ecs,0); // Empty world.
    auto wall=add(ecs,0,0,Collider::Box(10,10),false,0);
    auto moving=add(ecs,2,0,Collider::Box(10,10),false);
    auto sensor=add(ecs,18,0,Collider::Circle(5),true,0);
    physics.Update(ecs,0);
    check(ecs.GetComponent<CollisionState>(sensor).hit, "missed contact after cell crossing");
    check(ecs.GetComponent<ECS::Position>(moving).x == 10, "wall separation failed");
    ecs.DestroyEntity(wall); ecs.DestroyEntity(moving); ecs.DestroyEntity(sensor);
    physics.Update(ecs,0); // Reused buffers must not retain destroyed entities.
    auto large=add(ecs,-80,-80,Collider::Box(100,20),true,0,PI/4);
    auto small=add(ecs,-48,-48,Collider::Circle(5),true,0);
    physics.Update(ecs,0);
    check(ecs.GetComponent<CollisionState>(small).hit, "missed large rotated collider");
    ecs.GetComponent<CollisionState>(small).ignore = large;
    physics.Update(ecs,0);
    check(!ecs.GetComponent<CollisionState>(small).hit, "ignore filter failed");
    std::cout << "PASS: hash/brute-force coverage, boundaries, duplicates, empty rebuilds, cell crossing, rotated colliders, sensors and ignore filter\n";
}
