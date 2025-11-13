#include <iostream>
#include <string>
#include <format>
#include <fstream>
#include <chrono>

#include "MTVT.h"
#include "fbm.h"
#include "mesh_closest.h"
#include "benchmark.h"
#include "graphics.h"

using namespace std;
using namespace MTVT;

float sphereFunc(Vector3 v)
{
    return mag(v);
}

float fbmFunc(Vector3 v)
{
    return fbm(v * 2.0f, 3, 2.0f, 0.5f);
}

MappedMesh bunny_mesh;

int main()
{
    GraphicsEnv graphics;
    Mesh temp_mesh = {
        { { -0.5f, -0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { 0.0f,  0.5f, 0.0f } },
        {},
        {}
    };
    graphics.create(1024, 1024);

    auto result = runBenchmark("realtime test", 1, { -1, -1, -1 }, { 1, 1, 1 }, 0.2f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    graphics.setSummary(result.first);
    graphics.setMesh(result.second);

    while (graphics.draw())
    {

    }

    graphics.destroy();

    return 2;




    string csv_file = "benchmark;resolution x;resolution y;resolution z;iterations;lattice type;merge mode;threads;"
                      "sample points allocated;edges allocated;tetrahedra evaluated;vertices produced;triangles produced;indices produced;triangles discarded;"
                      "theoretical sample points;theoretical edges;total tetrahedra;"
                      "total time;allocation time;sampling time;vertex time;geometry time;"
                      "allocation time %;sampling time %;vertex time %;geometry time %;"
                      "sample point bytes;edge bytes;vertex buffer bytes;index buffer bytes;"
                      "sample point alloc relative;edge alloc relative;tetrahedra eval relative;"
                      "discarded tri fraction;verts per SP; verts per edge;verts per tetrahedron;tris per SP;tris per edge;tris per tetrahedron;"
                      "tri area mean;tri area max;tri area min;tri area SD;tri AR mean;tri AR max;tri AR min;tri AR SD\n";

    //csv_file += runBenchmark("sphere", 10, { -2, -2, -2 }, { 2, 2, 2 }, 0.04f, sphereFunc, 1.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("sphere", 10, { -2, -2, -2 }, { 2, 2, 2 }, 0.04f, sphereFunc, 1.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //csv_file += runBenchmark("fbm", 10, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("fbm", 10, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //csv_file += runBenchmark("bump", 10, { -4, -4, -4 }, { 4, 4, 4 }, 0.08f, [](Vector3 v) { return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z; }, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("bump", 10, { -4, -4, -4 }, { 4, 4, 4 }, 0.08f, [](Vector3 v) { return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z; }, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //
    //runBenchmark("fbm1", 10, { -1, -1, -1 }, { -0.5f, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //runBenchmark("fbm2", 10, { -0.5f, -1, -1 }, { 0, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //runBenchmark("fbm3", 10, { 0, -1, -1 }, { 0.5f, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //runBenchmark("fbm4", 10, { 0.5f, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //
    /*bunny_mesh.load("res/stanford_bunny/bunny_touchup.obj");

    runBenchmark("bunny", 1, { -0.1f, -0.06f, -0.01f }, { 0.1f, 0.08f, 0.16f }, 0.04f, [](Vector3 v) { return bunny_mesh.closestPointSDF(v); }, 0.0F, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    */
    auto current_time = chrono::current_zone()->to_local(chrono::system_clock::now());
    string filename = format("out/benchmark_{0:%d_%m_%Y %H.%M.%S}.csv", current_time);
    ofstream csv(filename);
    if (!csv.is_open())
        return 1;
    csv << csv_file;
    csv.close();

    return 0;
}
