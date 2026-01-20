#include "demo_functions.h"

#include "mesh_closest.h"
#include "fbm.h"

using namespace MTVT;
using namespace std;

float sphereFunc(Vector3 v)
{
    return 1.0f - mag(v);
}

float fbmFunc(Vector3 v)
{
    return fbm(v * 2.0f, 3, 2.0f, 0.5f);
}

float bumpFunc(Vector3 v)
{
    return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z;
}

float cubeFunc(Vector3 v)
{
    Vector3 q = abs(v) - Vector3{ 1.0f, 1.0f, 1.0f };
    return -(mag(max(q, Vector3{ 0.0f, 0.0f, 0.0f })) + min(max(q.x, max(q.y, q.z)), 0.0f));
}


MappedMesh bunny_mesh;
MappedMesh suzanne_mesh;

void bunnyInit()
{
    bunny_mesh.load("res/stanford_bunny/bunny_touchup.obj");
    suzanne_mesh.load("res/suzanne.obj");
}

float bunnyFunc(Vector3 v)
{
    return bunny_mesh.closestPointSDF(v);
}

float suzanneFunc(Vector3 v)
{
    return suzanne_mesh.closestPointSDF(v);
}
