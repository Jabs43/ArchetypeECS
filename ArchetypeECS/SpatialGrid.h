#pragma once
#include "raylib.h"
#include "ECS.h"
#include "Colliders.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

struct GridItem {
    ECS::EntityID id;
    Vector2 position;
    float hw, hh;
};

class SpatialGrid {
private:
    float cellSize;
    int gridWidth;
    int gridHeight;

    std::vector<std::vector<GridItem*>> cells;

    int clamp(int value, int min, int max) const;

public:
    SpatialGrid(float width, float height, float cellsize);

    int getCellIndex(float x, float y) const;
    void insert(GridItem* item);
    void clear();
    void getNearby(const GridItem* item, std::vector<GridItem*>& found) const;
    void build(std::vector<GridItem*>& items);
    void draw(const Rect& b) const;
};