#include "LedVisualizer.h"

LedVisualizer::LedVisualizer(LifeGame *game): Game{game}
{
  matrix->init();
  matrix->clear();
}

void LedVisualizer::display()
{
  for (uint8_t y = 0; y < Game->SizeY; y++)
  {
    for (uint8_t x = 0; x < Game->SizeX; x++)
    {
      uint8_t addr = x >> 3;
      if (Game->Data[x][y] == 1)
        matrix->Leds[addr][y] |= (1UL << (x % 8));
      else
        matrix->Leds[addr][y] &= ~(1UL << (x % 8));
    }
  }

  matrix->apply();
}
