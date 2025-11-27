#include <iostream>
#include <string>
#include <format>
#include <fstream>
#include <chrono>

#include "MTVT.h"
#include "mesh_closest.h"
#include "benchmark.h"
#include "graphics.h"
#include "demo_functions.h"

using namespace std;
using namespace MTVT;

MappedMesh bunny_mesh;

int main()
{
    bool start_gui = false;

    if (start_gui)
    {
        GraphicsEnv graphics;
        graphics.create(1024, 1024);

        auto result = runBenchmark("realtime test", 1, { -1, -1, -1 }, { 1, 1, 1 }, 0.05f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
        graphics.setSummary(result.first);
        graphics.setMesh(result.second, { 0, 0, 0 });

        while (graphics.draw());

        graphics.destroy();
    }
    else
    {
        auto result = runBenchmark("fbm", 50, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
        printBenchmarkSummary(result.first);
        result = runBenchmark("sphere", 50, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, sphereFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
        printBenchmarkSummary(result.first);
    }
    

    //csv_file += runBenchmark("sphere", 10, { -2, -2, -2 }, { 2, 2, 2 }, 0.04f, sphereFunc, 1.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("sphere", 10, { -2, -2, -2 }, { 2, 2, 2 }, 0.04f, sphereFunc, 1.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //csv_file += runBenchmark("fbm", 10, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("fbm", 10, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //csv_file += runBenchmark("bump", 10, { -4, -4, -4 }, { 4, 4, 4 }, 0.08f, [](Vector3 v) { return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z; }, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("bump", 10, { -4, -4, -4 }, { 4, 4, 4 }, 0.08f, [](Vector3 v) { return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z; }, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //
    /*bunny_mesh.load("res/stanford_bunny/bunny_touchup.obj");

    runBenchmark("bunny", 1, { -0.1f, -0.06f, -0.01f }, { 0.1f, 0.08f, 0.16f }, 0.04f, [](Vector3 v) { return bunny_mesh.closestPointSDF(v); }, 0.0F, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    */
    /*auto current_time = chrono::current_zone()->to_local(chrono::system_clock::now());
    string filename = format("out/benchmark_{0:%d_%m_%Y %H.%M.%S}.csv", current_time);
    ofstream csv(filename);
    if (!csv.is_open())
        return 1;
    csv << csv_file;
    csv.close();*/

    return 0;
}
