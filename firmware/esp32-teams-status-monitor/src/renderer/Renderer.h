#pragma once
#include <LedMatrix.h>

class Renderer
{
protected:
    

public:

    LedMatrix *mx;

    Renderer(LedMatrix *mx) : mx(mx){};
    virtual ~Renderer(){};

    virtual void init(){};
    virtual void display(){};
};