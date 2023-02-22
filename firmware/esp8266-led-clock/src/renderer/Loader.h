#pragma once

#include "Renderer.h"
#include <Ticker.h>

class Loader : public Renderer
{
private:
    Ticker tick;
    const String loading = "Loading. .  .   .    .";
    const unsigned short _maxPosX = SCREEN_CNT * 8 - 1;
    unsigned int _dPosX = _maxPosX;

public:
    Loader(LedMatrix *mx) : Renderer(mx){};

    void init() override;
    void stop();
};

void Loader::init()
{
    tick.attach(0.1, [&](){
        mx->scrollText(_dPosX++, loading);
        mx->apply(); 
    });
}

void Loader::stop()
{
    tick.detach();
}