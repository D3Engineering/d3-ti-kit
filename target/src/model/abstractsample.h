#pragma once

class AbstractSample
{
public:
    virtual void passImage(void* img) = 0;
    virtual float* getSample() = 0;
    virtual bool isReady() = 0;
    virtual void reset() = 0;
};
