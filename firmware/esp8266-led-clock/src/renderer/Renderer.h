#pragma once
#include <LedMatrix.h>

class Renderer
{
protected:

public:
    Renderer(LedMatrix *mx) : mx(mx){};
    virtual ~Renderer(){};

    LedMatrix *mx;
    virtual void init(){};
    virtual void display(){};
};