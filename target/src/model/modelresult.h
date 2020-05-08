#pragma once

#include <string>
#include <vector>
#include <map>

using namespace std;

struct ModelResult
{
    map<string, float> output;
    float cutoff;
};
