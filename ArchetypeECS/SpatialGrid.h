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

class SpatialHash {
private:
    // Large primes (Teschner et al.) decorrelate the two cell axes before masking.
    static constexpr uint32_t HASH_X = 73856093u;
    static constexpr uint32_t HASH_Y = 19349663u;

    float cellSize = 1.0f;
    float invCell = 1.0f;
    uint32_t mask = 0;                 // tableSize - 1 (tableSize is a power of two)

    std::vector<int> bucketStart;       // size table+1; slice of bucket b = [b]..[b+1]
    std::vector<int> cursor;            // size table; scatter write-heads (scratch)
    std::vector<GridItem*> entries;     // size n; item pointers grouped by bucket

    int cellX(float x) const;
    int cellY(float y) const;

    uint32_t bucketOf(int cx, int cy) const;

public:
    // Rebuild from current centers; cell size follows the largest half-extent.
    // Item positions must remain unchanged until all queries are complete.
    void build(std::vector<GridItem*>& items);

    void clear();

    /* Broad phase: replace found with every item whose CENTER lies inside `range`.
       Walks only the cells the range touches. The cell-match test rejects items
       that merely collided into the same bucket from a different cell (and, since
       each cell is visited once and an item belongs to exactly one cell, also
       guarantees no item is reported twice). */
    void query(const Rect& range, std::vector<GridItem*>& found) const;

    void draw(const Rect& b) const;
};