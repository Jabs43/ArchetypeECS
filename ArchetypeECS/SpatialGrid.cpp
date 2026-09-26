#include "SpatialGrid.h"

int SpatialGrid::clamp(int value, int min, int max) const {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

SpatialGrid::SpatialGrid(float width, float height, float cellsize)
    : cellSize(cellsize) {
    gridWidth = static_cast<int>(std::ceil(width / cellSize));
    gridHeight = static_cast<int>(std::ceil(height / cellSize));
    cells.resize(gridWidth * gridHeight);
}

int SpatialGrid::getCellIndex(float x, float y) const {
    int cellX = clamp(static_cast<int>(x / cellSize), 0, gridWidth - 1);
    int cellY = clamp(static_cast<int>(y / cellSize), 0, gridHeight - 1);
    return cellX + cellY * gridWidth;
}

void SpatialGrid::insert(GridItem* item) {
    int index = getCellIndex(item->position.x, item->position.y);
    cells[index].push_back(item);
}

void SpatialGrid::clear() {
    for (auto& cell : cells) {
        cell.clear();
    }
}

void SpatialGrid::getNearby(const GridItem* item, std::vector<GridItem*>& found) const {
    found.clear();
    int centerCellx = clamp(static_cast<int>(item->position.x / cellSize), 0, gridWidth - 1);
    int centerCelly = clamp(static_cast<int>(item->position.y / cellSize), 0, gridHeight - 1);
    for (int y = centerCelly - 1; y <= centerCelly + 1; ++y) {
        if (y < 0 || y >= gridHeight) continue;
        for (int x = centerCellx - 1; x <= centerCellx + 1; ++x) {
            if (x < 0 || x >= gridWidth) continue;
            int index = x + y * gridWidth;
            for (GridItem* neighbor : cells[index]) {
                if (neighbor->id != item->id) {
                    found.push_back(neighbor);
                }
            }
        }
    }
}

void SpatialGrid::build(std::vector<GridItem*>& items) {
    clear();
    for (auto* q : items) {
        insert(q);
    }
}

void SpatialGrid::draw(const Rect& b) const {
    if (cellSize <= 0.0f) return;
    for (float x = b.x; x <= b.x + b.width; x += cellSize)
        DrawLineV({ x, b.y }, { x, b.y + b.height }, GREEN);
    for (float y = b.y; y <= b.y + b.height; y += cellSize)
        DrawLineV({ b.x, y }, { b.x + b.width, y }, GREEN);
    //for (auto* it : entries)
    //    DrawCircleV({ it->position.x, it->position.y }, 2.0f, RED);
}

int SpatialHash::cellX(float x) const { return static_cast<int>(std::floor(x * invCell)); }
int SpatialHash::cellY(float y) const { return static_cast<int>(std::floor(y * invCell)); }

uint32_t SpatialHash::bucketOf(int cx, int cy) const {
    uint32_t h = static_cast<uint32_t>(cx) * HASH_X
        ^ static_cast<uint32_t>(cy) * HASH_Y;
    return h & mask;
}

// Rebuild from the current solver pass. Each item is stored by its center.
void SpatialHash::build(std::vector<GridItem*>& items) {
    const int n = static_cast<int>(items.size());

    // 1. cell size = largest half-extent, with a minimum of one unit
    float maxHalf = 0.0f;
    for (auto* it : items) maxHalf = std::max({ maxHalf, it->hw, it->hh });
    cellSize = std::max(maxHalf, 1.0f);
    invCell = 1.0f / cellSize;

    // 2. table = next power of two >= 2n (load factor ~0.5), min 16
    uint32_t table = 16;
    while (table < static_cast<uint32_t>(2 * n)) table <<= 1;
    mask = table - 1;

    // 3. count items per bucket (offset by one for the prefix sum)
    bucketStart.assign(table + 1, 0);   // reuses capacity if table unchanged
    for (auto* it : items)
        bucketStart[bucketOf(cellX(it->position.x), cellY(it->position.y)) + 1]++;

    // 4. prefix sum -> bucketStart[b] is bucket b's start offset
    for (uint32_t b = 0; b < table; ++b) bucketStart[b + 1] += bucketStart[b];

    // 5. scatter item pointers into their bucket's slice
    cursor.assign(bucketStart.begin(), bucketStart.begin() + table);
    entries.resize(n);
    for (auto* it : items)
        entries[cursor[bucketOf(cellX(it->position.x), cellY(it->position.y))]++] = it;
}

void SpatialHash::clear() {
    bucketStart.clear();
    cursor.clear();
    entries.clear();
    mask = 0;
}

// Broad phase: collect every item whose CENTER lies inside `range`.
// Walks only the cells the range touches. The cell-match test rejects items
// that merely collided into the same bucket from a different cell (and, since
// each cell is visited once and an item belongs to exactly one cell, also
// guarantees no item is reported twice).
void SpatialHash::query(const Rect& range, std::vector<GridItem*>& found) const {
    found.clear();
    if (entries.empty()) return;
    const int cx0 = cellX(range.x), cx1 = cellX(range.x + range.width);
    const int cy0 = cellY(range.y), cy1 = cellY(range.y + range.height);

    for (int cy = cy0; cy <= cy1; ++cy)
        for (int cx = cx0; cx <= cx1; ++cx) {
            const uint32_t b = bucketOf(cx, cy);
            for (int i = bucketStart[b]; i < bucketStart[b + 1]; ++i) {
                GridItem* it = entries[i];
                if (cellX(it->position.x) == cx && cellY(it->position.y) == cy
                    && range.contains(it->position))
                    found.push_back(it);
            }
        }
}

void SpatialHash::draw(const Rect& b) const {
    if (cellSize <= 0.0f) return;
    for (float x = b.x; x <= b.x + b.width; x += cellSize)
        DrawLineV({ x, b.y }, { x, b.y + b.height }, GREEN);
    for (float y = b.y; y <= b.y + b.height; y += cellSize)
        DrawLineV({ b.x, y }, { b.x + b.width, y }, GREEN);
    //for (auto* it : entries)
    //    DrawCircleV({ it->position.x, it->position.y }, 2.0f, RED);
}
