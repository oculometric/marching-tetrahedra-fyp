#include "demo_functions.h"

#include "fbm.h"

using namespace MTVT;

float sphereFunc(Vector3 v)
{
    return mag(v);
}

float fbmFunc(Vector3 v)
{
    return fbm(v * 2.0f, 3, 2.0f, 0.5f);
}

float bumpFunc(MTVT::Vector3 v)
{
    return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z;
}
