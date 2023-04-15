#pragma once

#include <LedMatrix.h>
#include "Renderer.h"
#include "../TimeSource.h"

#define SCREEN_MIN_BRIGHTNESS 0x01
#define SCREEN_MAX_BRIGHTNESS 0x0b

class AutoBrightness: public Renderer
{
private:
    TimeSource* time;
    uint8_t screen_current_brightness = SCREEN_MIN_BRIGHTNESS;
    uint8_t screen_desired_brightness = SCREEN_MIN_BRIGHTNESS;
    
    uint8_t calcBrightness(uint8_t hour);

public:
    AutoBrightness(LedMatrix *mx, TimeSource* time) 
        : Renderer(mx), time(time) {};

    void init() override;
    void display() override;
};

void AutoBrightness::init()
{
    mx->setBrightness(SCREEN_MIN_BRIGHTNESS);
}

uint8_t AutoBrightness::calcBrightness(uint8_t hour) {
    if (hour >= 20)
        return SCREEN_MIN_BRIGHTNESS;
    else if (hour >= 16)
        return map(hour, 16, 20, SCREEN_MAX_BRIGHTNESS, SCREEN_MIN_BRIGHTNESS);
    else if (hour >= 8)
        return SCREEN_MAX_BRIGHTNESS;
    else if (hour >= 4)
        return map(hour, 4, 8, SCREEN_MIN_BRIGHTNESS, SCREEN_MAX_BRIGHTNESS);
    else
        return SCREEN_MIN_BRIGHTNESS;
}

void AutoBrightness::display()
{
    uint8_t hour = timeSource.get()->tm_hour;
    screen_desired_brightness = calcBrightness(hour);
    if (screen_current_brightness > screen_desired_brightness)
        mx->setBrightness(--screen_current_brightness);
    else if (screen_current_brightness < screen_desired_brightness)
        mx->setBrightness(++screen_current_brightness);
}