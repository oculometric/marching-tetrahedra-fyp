#include <iostream>
#include <string>
#include <format>
#include <fstream>
#include <chrono>

#include "MTVT.h"
#include "benchmark.h"
#include "graphics.h"
#include "demo_functions.h"

using namespace std;
using namespace MTVT;

int main()
{
    bool start_gui = false;
    bunnyInit();

    if (start_gui)
    {
        GraphicsEnv graphics;
        graphics.create(1024, 1024);

        while (graphics.draw());

        graphics.destroy();
    }
    else
    {
        string csv;
        csv += generateCSVLine({}, true);

        const int iterations = 100;

        auto result = runBenchmark("sphere_bu", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, sphereFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/sphere_bu");

        result = runBenchmark("sphere_bi", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, sphereFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/sphere_bi");

        result = runBenchmark("sphere_bp", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, sphereFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::POST_PROCESED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/sphere_bp");

        result = runBenchmark("sphere_su", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, sphereFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::NONE, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/sphere_su");

        result = runBenchmark("sphere_si", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, sphereFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::INTEGRATED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/sphere_si");

        result = runBenchmark("sphere_sp", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, sphereFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::POST_PROCESED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/sphere_sp");


        result = runBenchmark("asteroid_bu", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, asteroidFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/asteroid_bu");

        result = runBenchmark("asteroid_bi", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, asteroidFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/asteroid_bi");

        result = runBenchmark("asteroid_bp", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, asteroidFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::POST_PROCESED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/asteroid_bp");

        result = runBenchmark("asteroid_su", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, asteroidFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::NONE, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/asteroid_su");

        result = runBenchmark("asteroid_si", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, asteroidFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::INTEGRATED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/asteroid_si");

        result = runBenchmark("asteroid_sp", iterations, { -1, -1, -1 }, { 1, 1, 1 }, 0.04f, asteroidFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::POST_PROCESED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/asteroid_sp");

        const Vector3 bunny_min = { -0.7f, -0.4f, -0.5f };
        const Vector3 bunny_max = { 0.8f, 0.4f, 0.6f };
        const float bunny_res = 0.02f;
        result = runBenchmark("bunny_bu", iterations, bunny_min, bunny_max, bunny_res, bunnyFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/bunny_bu");

        result = runBenchmark("bunny_bi", iterations, bunny_min, bunny_max, bunny_res, bunnyFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/bunny_bi");

        result = runBenchmark("bunny_bp", iterations, bunny_min, bunny_max, bunny_res, bunnyFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::POST_PROCESED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/bunny_bp");

        result = runBenchmark("bunny_su", iterations, bunny_min, bunny_max, bunny_res, bunnyFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::NONE, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/bunny_su");

        result = runBenchmark("bunny_si", iterations, bunny_min, bunny_max, bunny_res, bunnyFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::INTEGRATED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/bunny_si");

        result = runBenchmark("bunny_sp", iterations, bunny_min, bunny_max, bunny_res, bunnyFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::POST_PROCESED, 16);
        printBenchmarkSummary(result.first);
        csv += generateCSVLine(result.first, false);
        dumpMeshToOBJ(result.second, "benchmark/bunny_sp");


        auto current_time = chrono::current_zone()->to_local(chrono::system_clock::now());
        string filename = format("out/benchmark_{0:%d_%m_%Y %H.%M.%S}.csv", current_time);
        ofstream csv_file(filename);
        if (!csv_file.is_open())
            return 1;
        csv_file << csv;
        csv_file.close();
    }

    return 0;
}
