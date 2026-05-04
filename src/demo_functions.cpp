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

float asteroidFunc(Vector3 v)
{
    Vector3 factor = Vector3{ 0.9f, 1.0f, 0.9f };
    float rad = mag(v / factor);
    return (0.65f - rad) + (0.35f * fbm((norm(v) / factor) * 1.5f, 3, 3.0f, 0.15f));
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

float fract(float f)
{
    return f - floor(f);
}

float voronoiHash(Vector3 v)
{
    return (fract(sin((v ^ Vector3(201.0f, 123.0f, 304.2f))) * 190493.02095f) * 2.0f) - 1.0f;
}

// returns the distance metric for a given position in 3D voronoi nosie
float voronoi(Vector3 position, float randomness)
{
    Vector3 cell = floor(position);
    float closest = 4.0f;
    Vector3 closest_cell = Vector3(0.0f, 0.0f, 0.0f);
    float max_rand = ceil(randomness);

    for (float z = cell.z - max_rand; z <= cell.z + max_rand; z += 1.0f)
    {
        for (float y = cell.y - max_rand; y <= cell.y + max_rand; y += 1.0f)
        {
            for (float x = cell.x - max_rand; x <= cell.x + max_rand; x += 1.0f)
            {
                Vector3 test_cell = Vector3(x, y, z);
                test_cell += Vector3(voronoiHash(Vector3(x, y, z)), voronoiHash(Vector3(y, z, x)), voronoiHash(Vector3(z, x, y))) * randomness * 0.5f;

                float dist = mag(position - test_cell);
                if (dist < closest)
                {
                    closest = dist;
                    closest_cell = Vector3(x, y, z);
                }
            }
        }
    }

    return closest;
}

#include <chrono>

float getTime()
{
    static auto begin = chrono::steady_clock::now();

    auto now = chrono::steady_clock::now();
    chrono::duration<float> diff = now - begin;
    return diff.count();
}

float blobsFunc(Vector3 v)
{
    float vdot = v ^ v;
    float f = (fbm(v, 2, 2.0f, 0.5f) * 0.2f) + 0.3f;
    float f2 = (fbm(v * 3.0f, 3, 2.0f, 0.5f) * 0.5f) + 0.2f;
    float o = voronoi((v * 2.0f) + (Vector3(cos(getTime() * 0.3f), cos(getTime() * 0.01f), sin(getTime()) * f)), 1.0f);
    return (-(((o * 2.0f * powf(vdot, 0.5f)) - (0.3f + (0.2f * sin(getTime() * 1.5f)))) + (powf(vdot, 3.0f) * 0.5f))) + f2;
}
