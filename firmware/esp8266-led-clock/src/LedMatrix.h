#pragma once

#include <inttypes.h>
#include <WString.h>

class LedMatrix
{
public:
    LedMatrix(uint8_t count) : Count{count} {};
    ~LedMatrix();

    void init();
    void setBrightness(uint8_t);
    void clear();
    void apply();

    
    uint16_t scrollText(int16_t posX, String txt);
    uint8_t char2Arr_p(uint16_t ch, int PosX);
    uint8_t char2Arr_t(unsigned short ch, int PosX, short PosY);

    uint8_t Count;
    uint8_t Leds[SCREEN_CNT][8] = {};

protected:
    void dumpToConsole();
    
private:
};