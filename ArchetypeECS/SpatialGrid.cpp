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
