#include <Arduino.h>
#include "LifeGame.h"

void LifeGame::fillRandom()
{
  for (uint8_t x = 0; x < SizeX; x++)
    for (uint8_t y = 0; y < SizeY; y++)
    {
      uint8_t val = random(2); 
      Data[x][y] = val;
    }
}

void LifeGame::copyGrid(Grid_t from, Grid_t to)
{
  for (uint8_t x = 0; x < SizeX; x++)
    for (uint8_t y = 0; y < SizeY; y++)
      to[x][y] = from[x][y];
}

bool LifeGame::isGridEqual(Grid_t from, Grid_t to)
{
  for (uint8_t x = 0; x < SizeX; x++)
    for (uint8_t y = 0; y < SizeY; y++)
      if (to[x][y] != from[x][y])
        return false;
  return true;
}

uint8_t LifeGame::gridPopulation(Grid_t arr)
{
  uint8_t result = 0;
  for (uint8_t x = 0; x < SizeX; x++)
    for (uint8_t y = 0; y < SizeY; y++)
      result += arr[x][y];
  return result;
}

void LifeGame::doStep()
{
  step++;

  Grid_t Copy = {};
  copyGrid(Data, Copy);

  uint8_t alive = 0;
  for (uint8_t x = 0; x < SizeX; x++)
  {
    for (uint8_t y = 0; y < SizeY; y++)
    {
      alive = 0;
      for (int8_t c = -1; c < 2; c++)
      {
        for (int8_t d = -1; d < 2; d++)
          if (
              !(c == 0 && d == 0) && ((x + c) >= 0) && ((y + d) >= 0) && ((x + c) < SizeX) && ((y + d) < SizeY))
            if (Copy[x + c][y + d] == 1)
              ++alive;
      }

      if (alive < 2)
        Data[x][y] = 0;
      else if (alive == 3)
        Data[x][y] = 1;
      else if (alive > 3)
        Data[x][y] = 0;
    }
  }

  // check if we're looped
  if (isGridEqual(Data, Copy))
    emptyMovesCount++;
  else
    emptyMovesCount = 0;

  // check if empty moves count off limit
  if (emptyMovesCount >= emptyMovesThreshold)
  {
    emptyMovesCount = 0;
    fillRandom();
    return;
  } 

  uint8_t population = gridPopulation(Data);
  if (population == lastPopulation)
    loopMovesCount++;
  else
    loopMovesCount = 0;
  lastPopulation = population;

  if (loopMovesCount >= loopMovesThreshold) {
    loopMovesCount = 0;
    fillRandom();
    return;
  }

}