#pragma once
#include <LedMatrix.h>

class Renderer
{
protected:
    LedMatrix *mx;

public:
    Renderer(LedMatrix *mx) : mx(mx){};
    virtual ~Renderer(){};

    virtual void init(){};
    virtual void display(){};
};