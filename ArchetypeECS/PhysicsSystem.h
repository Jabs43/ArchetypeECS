#pragma once
#include "ECS.h"
#include "Colliders.h"
#include "SpatialGrid.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>

#ifdef PHYS_PROFILE
#include "PhysProfiler.h"
#include <chrono>
#endif

// Sensor contacts report hits without applying impulses (used by bullets).
struct CollisionState {
    bool hit = false;
    bool sensor = false;
    ECS::EntityID ignore = std::numeric_limits<ECS::EntityID>::max();
};

namespace System {
class PhysicsSystem {
    struct ShapeData {
        Collider::Shape shape;
        float hw, hh, radius, rotation;
    };
    struct Body {
        GridItem grid;
        ECS::Velocity velocity;
        ShapeData shape;
        RigidBody material;
        CollisionState collision;
    };
    Rect arena_;
    std::unique_ptr<SpatialHash> grid_;
    std::vector<Body> bodies_;
    std::vector<GridItem*> pointers_;
    std::vector<GridItem*> nearby_;
    std::vector<std::pair<size_t, size_t>> candidates_;
    std::unordered_map<ECS::EntityID, size_t> indices_;

#ifdef PHYS_PROFILE
    PhysProfiler mProfiler;
#endif

    static Contact circleCircle(Vector2 pa, float ra, Vector2 pb, float rb) {
        const float dx = pb.x - pa.x, dy = pb.y - pa.y;
        const float radius = ra + rb;
        const float distance2 = dx * dx + dy * dy;
        if (distance2 > radius * radius) return {};
        const float distance = std::sqrt(distance2);
        // Coincident centers have no preferred normal; choose a stable axis.
        const Vector2 normal = distance > 0.0f
            ? Vector2{dx / distance, dy / distance} : Vector2{1.0f, 0.0f};
        return {true, normal, radius - distance};
    }

    static Contact boxCircle(Vector2 boxC, const ShapeData& box, Vector2 circleC, float radius) {
        const float co = std::cos(box.rotation), si = std::sin(box.rotation);
        const float dx = circleC.x - boxC.x, dy = circleC.y - boxC.y;
        const float x = dx * co + dy * si, y = -dx * si + dy * co;
        const float ex = x - std::clamp(x, -box.hw, box.hw);
        const float ey = y - std::clamp(y, -box.hh, box.hh);
        const float distance2 = ex * ex + ey * ey;
        if (distance2 > radius * radius) return {};

        Vector2 normal{};
        float depth;
        if (distance2 > 0.0f) {
            const float distance = std::sqrt(distance2);
            normal = {ex / distance, ey / distance};
            depth = radius - distance;
        } else {
            // Center inside/on the box: escape through the nearest face.
            const float toX = box.hw - std::fabs(x);
            const float toY = box.hh - std::fabs(y);
            if (toX <= toY) {
                normal = {x >= 0.0f ? 1.0f : -1.0f, 0.0f};
                depth = radius + toX;
            } else {
                normal = {0.0f, y >= 0.0f ? 1.0f : -1.0f};
                depth = radius + toY;
            }
        }
        return {true, {normal.x * co - normal.y * si,
                       normal.x * si + normal.y * co}, depth};
    }

    static Contact boxBoxSAT(Vector2 pa, const ShapeData& a, Vector2 pb, const ShapeData& b) {
        const float ca = std::cos(a.rotation), sa = std::sin(a.rotation);
        const float cb = std::cos(b.rotation), sb = std::sin(b.rotation);
        const Vector2 axes[4] = {{ca, sa}, {-sa, ca}, {cb, sb}, {-sb, cb}};
        const Vector2 d{pb.x - pa.x, pb.y - pa.y};
        Contact result{true, {}, std::numeric_limits<float>::max()};
        for (Vector2 axis : axes) {
            const float ra = a.hw * std::fabs(axis.x * ca + axis.y * sa)
                           + a.hh * std::fabs(-axis.x * sa + axis.y * ca);
            const float rb = b.hw * std::fabs(axis.x * cb + axis.y * sb)
                           + b.hh * std::fabs(-axis.x * sb + axis.y * cb);
            const float signedDistance = d.x * axis.x + d.y * axis.y;
            // Distance to separate intervals, including full containment.
            const float depth = ra + rb - std::fabs(signedDistance);
            if (depth < 0.0f) return {};
            if (depth < result.depth) {
                result.depth = depth;
                result.normal = signedDistance >= 0.0f
                    ? axis : Vector2{-axis.x, -axis.y};
            }
        }
        return result;
    }

    static void clampAxis(float& position, float& velocity,
                          float halfExtent, float minimum, float maximum) {
        if (2.0f * halfExtent >= maximum - minimum) {
            // No room to fit along this axis; avoid alternating wall snaps.
            position = minimum + (maximum - minimum) * 0.5f;
            velocity = 0.0f;
        } else if (position - halfExtent < minimum) {
            position = minimum + halfExtent;
            if (velocity < 0.0f) velocity = -velocity;
        } else if (position + halfExtent > maximum) {
            position = maximum - halfExtent;
            if (velocity > 0.0f) velocity = -velocity;
        }
    }

    static Contact contact(const Body& a, const Body& b) {
        const auto pa = a.grid.position, pb = b.grid.position;
        if (std::fabs(pb.x-pa.x) > a.grid.hw+b.grid.hw ||
            std::fabs(pb.y-pa.y) > a.grid.hh+b.grid.hh) return {};
        if (a.shape.shape == Collider::Shape::Circle && b.shape.shape == Collider::Shape::Circle)
            return circleCircle(pa, a.shape.radius, pb, b.shape.radius);
        if (a.shape.shape == Collider::Shape::Box && b.shape.shape == Collider::Shape::Box)
            return boxBoxSAT(pa, a.shape, pb, b.shape);
        if (a.shape.shape == Collider::Shape::Box)
            return boxCircle(pa, a.shape, pb, b.shape.radius);
        auto c = boxCircle(pb, b.shape, pa, a.shape.radius);
        c.normal = {-c.normal.x, -c.normal.y};
        return c;
    }

    static bool resolve(Body& a, Body& b) {
        if (a.collision.ignore == b.grid.id || b.collision.ignore == a.grid.id) return false;
        const auto c = contact(a, b);
        if (!c.hit) return false;
        a.collision.hit = b.collision.hit = true;
        if (a.collision.sensor || b.collision.sensor) return true;
        const float ma = std::max(0.0f, a.material.inversMass);
        const float mb = std::max(0.0f, b.material.inversMass);
        const float sum = ma+mb;
        if (sum == 0) return true;
        a.grid.position.x -= c.normal.x*c.depth*ma/sum;
        a.grid.position.y -= c.normal.y*c.depth*ma/sum;
        b.grid.position.x += c.normal.x*c.depth*mb/sum;
        b.grid.position.y += c.normal.y*c.depth*mb/sum;
        const float speed = (b.velocity.x-a.velocity.x)*c.normal.x
                          + (b.velocity.y-a.velocity.y)*c.normal.y;
        if (speed >= 0) return true;
        const float restitution = std::clamp(std::min(a.material.restitution, b.material.restitution), 0.0f, 1.0f);
        const float impulse = -(1+restitution)*speed/sum;
        a.velocity.x -= impulse*ma*c.normal.x;
        a.velocity.y -= impulse*ma*c.normal.y;
        b.velocity.x += impulse*mb*c.normal.x;
        b.velocity.y += impulse*mb*c.normal.y;
        return true;
    }

    void constrain(Body& body) const {
        if (body.material.inversMass <= 0) return;
        auto& p = body.grid.position;
        if (body.collision.sensor &&
            (p.x-body.grid.hw < arena_.x || p.x+body.grid.hw > arena_.x+arena_.width ||
             p.y-body.grid.hh < arena_.y || p.y+body.grid.hh > arena_.y+arena_.height))
            body.collision.hit = true;
        clampAxis(p.x, body.velocity.x, body.grid.hw, arena_.x, arena_.x+arena_.width);
        clampAxis(p.y, body.velocity.y, body.grid.hh, arena_.y, arena_.y+arena_.height);
    }

public:
    explicit PhysicsSystem(Rect arena) : arena_(arena) {
        grid_ = std::make_unique<SpatialHash>();
    }

#ifdef PHYS_PROFILE
    void ReportProfile() const { mProfiler.report("physics updates"); }
    void ResetProfile() { mProfiler.reset(); }
#endif

    void Update(ECS::ArchetypeRegistry& ecs, float dt) {
#ifdef PHYS_PROFILE
        using _clk = std::chrono::steady_clock;
        PhysSplit fs{};
        std::uint64_t pairs = 0, overlaps = 0;
        auto _t = _clk::now();
        auto _lap = [&](double& acc) { auto _n = _clk::now(); acc += std::chrono::duration<double, std::milli>(_n - _t).count(); _t = _n; };
#endif
        // ECSraylib applies acceleration before predicting positions. Here it
        // is optional, so existing archetypes need no additional component.
        ecs.ForEach<ECS::Position, ECS::Velocity, Rotation, Collider, RigidBody,
                    CollisionState, ECS::Acceleration>(
            [&](ECS::EntityID, ECS::Position&, ECS::Velocity& v, Rotation&,
                Collider&, RigidBody& material, CollisionState&, ECS::Acceleration& a) {
                if (material.inversMass > 0) {
                    v.x += a.x * dt;
                    v.y += a.y * dt;
                }
            });
        bodies_.clear(); indices_.clear();
        float maxHalfX = 0, maxHalfY = 0;
        ecs.ForEach<ECS::Position, ECS::Velocity, Rotation, Collider, RigidBody, CollisionState>(
            [&](ECS::EntityID id, ECS::Position& p, ECS::Velocity& v, Rotation& r,
                Collider& collider, RigidBody& material, CollisionState& collision) {
                const auto bounds = collider.BoundingHalfExtents(r.radians);
                const auto half = collider.GetBoxHalfExtents();
                Body body{{id, {p.x, p.y}, bounds.x, bounds.y}, v,
                    {collider.GetShape(), half.x, half.y, collider.GetCircleRadius(), r.radians},
                    material, collision};
                body.collision.hit = false;
                if (material.inversMass > 0) {
                    body.grid.position.x += v.x*dt;
                    body.grid.position.y += v.y*dt;
                } else {
                    body.velocity = {0, 0};
                }
                constrain(body);
                maxHalfX = std::max(maxHalfX, bounds.x);
                maxHalfY = std::max(maxHalfY, bounds.y);
                indices_[id] = bodies_.size();
                bodies_.push_back(body);
            });

#ifdef PHYS_PROFILE
        _lap(fs.snapshot);
#endif

        pointers_.clear();
        pointers_.reserve(bodies_.size());
        for (auto& body : bodies_) pointers_.push_back(&body.grid);

        // Match ECSraylib's active PhysicsSystemSG: one build and one query
        // phase at predicted positions, then two passes over a fixed pair list.
        grid_->build(pointers_);
#ifdef PHYS_PROFILE
        _lap(fs.build);
#endif
        candidates_.clear();
        for (size_t i = 0; i < bodies_.size(); ++i) {
            const auto& a = bodies_[i];
            const float rx = a.grid.hw + maxHalfX, ry = a.grid.hh + maxHalfY;
            grid_->query({a.grid.position.x - rx, a.grid.position.y - ry,
                          2.0f * rx, 2.0f * ry}, nearby_);
            for (const auto* b : nearby_) {
                if (b->id > a.grid.id)
                    candidates_.emplace_back(i, indices_.at(b->id));
            }
        }
#ifdef PHYS_PROFILE
        _lap(fs.query);
        pairs = candidates_.size();
#endif
        // Recompute contact geometry after earlier corrections. A contact
        // outside this initial candidate list is discovered on the next update.
        constexpr int ResolveIterations = 2;
        for (int iteration = 0; iteration < ResolveIterations; ++iteration) {
            for (const auto& [a, b] : candidates_) {
                const bool hit = resolve(bodies_[a], bodies_[b]);
#ifdef PHYS_PROFILE
                if (iteration == 0 && hit) ++overlaps;
#else
                (void)hit;
#endif
            }
        }
#ifdef PHYS_PROFILE
        _lap(fs.narrow);
#endif

        // No ECS structural mutations occurred while these snapshots existed.
        for (auto& body : bodies_) {
            constrain(body);
            ecs.GetComponent<ECS::Position>(body.grid.id) = {body.grid.position.x, body.grid.position.y};
            ecs.GetComponent<ECS::Velocity>(body.grid.id) = body.velocity;
            ecs.GetComponent<CollisionState>(body.grid.id) = body.collision;
        }
#ifdef PHYS_PROFILE
        _lap(fs.writeback);
        mProfiler.addFrame(fs, bodies_.size(), pairs, overlaps);
#endif
    }

    void DrawGrid() { if (grid_) grid_->draw(arena_); }
};
} // namespace System
