#pragma once

#include <inttypes.h>
#include "LifeGame.h"
#include "LedMatrix.h"

class LedVisualizer
{
public:
    LedVisualizer(LifeGame *game);
    void display();

private:
    LedMatrix *matrix = new LedMatrix(SCREEN_CNT);
    LifeGame *Game;
};