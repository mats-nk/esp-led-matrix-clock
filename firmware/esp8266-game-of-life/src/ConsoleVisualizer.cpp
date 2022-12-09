#include <stdint.h>
#include <Arduino.h>
#include "ConsoleVisualizer.h"

void ConsoleVisualizer::display()
{
  Serial.printf("\nStep=%d", Game->step);
  for (int8_t y = Game->SizeY - 1; y >= 0; y--)
  {
    Serial.printf("\n%d", y);
    for (uint8_t x = 0; x < Game->SizeX; x++)
    {
      if (x % 8 == 0) Serial.print("|");
      if (Game->Data[x][y] == 0)
        Serial.print('.');
      else
        Serial.print('O');
      // delay(5);
    }
    delay(10);
  }
  Serial.println(' ');
}
