#ifndef CONSOLEVISU_H
#define CONSOLEVISU_H

#include "LifeGame.h"

class ConsoleVisualizer
{
public:
    ConsoleVisualizer(LifeGame *game) : Game{game} {};
    void display();

private:
    LifeGame *Game;
};

#endif