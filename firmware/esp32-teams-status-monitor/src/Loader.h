#pragma once

#include "fonts.h"
#include "LedMatrix.h"
#include <Ticker.h>

class Loader
{
private:
    Ticker tick;
    bool _tick_attached = false;
    const unsigned short _maxPosX = SCREEN_CNT * 8;
    unsigned int _dPosX = _maxPosX - 1;
    bool directionLeft = true;

public:
    LedMatrix *mx;
    String text;
    unsigned int getPosition();

    Loader(LedMatrix *mx) : mx(mx){};

    void scroll(String text);
    void stop();
};

void __loader_ticker_callback(Loader *self)
{
    self->mx->scrollText(self->getPosition(), self->text);
    self->mx->apply();
}

void Loader::scroll(String _text)
{
    text = _text;
    mx->clear();

    if (!_tick_attached)
    {
        tick.attach_ms(500, __loader_ticker_callback, this);
        _tick_attached = true;
    }
}

unsigned int Loader::getPosition()
{
    uint8_t text_length = SYMBOL_WIDTH * text.length() - 1;
    log_v("text_length = %d, _maxPosX = %d", text_length, _maxPosX);
    if (text_length <= _maxPosX)
        // text shorter than screen - display all in the middle
        return _maxPosX - (_maxPosX - text_length) / 2;
    else
    {
        if (directionLeft)
        {
            _dPosX++;
            if (_dPosX > text_length + SYMBOL_WIDTH)
                directionLeft = !directionLeft;
        }
        else
        {
            _dPosX--;
            if (_dPosX < _maxPosX)
                directionLeft = !directionLeft;
        }
        return _dPosX;
    }
}

void Loader::stop()
{
    tick.detach();
    _tick_attached = false;
}