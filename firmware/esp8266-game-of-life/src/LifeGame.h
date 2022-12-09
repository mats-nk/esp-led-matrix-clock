#pragma once

#include <inttypes.h>

#define GRID_SIZE_X (8 * SCREEN_CNT)
#define GRID_SIZE_Y 8

typedef uint8_t Grid_t[GRID_SIZE_X][GRID_SIZE_Y];

class LifeGame 
{
public:
    LifeGame() {};
    void doStep();
    void fillRandom();

    uint16_t step = 0;
    uint8_t SizeX = GRID_SIZE_X; 
    uint8_t SizeY = GRID_SIZE_Y;
    Grid_t Data = {};

private:
    void copyGrid(Grid_t from, Grid_t to);
    bool isGridEqual(Grid_t from, Grid_t to);
    uint8_t gridPopulation(Grid_t arr);

    uint8_t emptyMovesCount = 0;
    const uint8_t emptyMovesThreshold = 5;

    uint8_t lastPopulation = 0;
    uint8_t loopMovesCount = 0;
    const uint8_t loopMovesThreshold = 10;
};