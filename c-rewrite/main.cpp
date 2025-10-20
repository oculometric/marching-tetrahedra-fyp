#include <iostream>
#include <string>
#include <format>

#include "src/MTVT.h"

using namespace std;

void runBenchmark(string name, int iterations, Vector3 min, Vector3 max, float cube_size, float (*sampler)(Vector3), float threshold)
{
    MTVTBuilder builder;
    builder.configure(min, max, cube_size, sampler, threshold);
    MTVTDebugStats stats;

    for (int i = 0; i < iterations; ++i)
    {
        cout << format("{0} test iteration {1}/{2}\r", name, i, iterations);
        builder.generate(stats);
    }

    cout << '\b';
    cout <<        "-- summary -----------------------------" << endl;
    cout << format("  {0} test ({1} iterations)", name, iterations) << endl;
    cout << format("  {0}x{1}x{2} resolution", stats.cubes_x, stats.cubes_y, stats.cubes_z) << endl;
    cout <<        "  results:" << endl;
    cout << format("    sample points:  {0}", stats.sample_points) << endl;
    cout << format("    edges:          {0}", stats.edges) << endl;
    cout << format("    tetrahedra:     {0}", stats.tetrahedra) << endl;
    cout << format("    vertices:       {0}", stats.vertices) << endl;
    cout << format("    indices:        {0}", stats.indices) << endl;
    cout <<        "  performance:" << endl;
    cout << format("    allocation:     {0:.6f}s", stats.allocation_time / iterations) << endl;
    cout << format("    sampling:       {0:.6f}s", stats.sampling_time / iterations) << endl;
    cout << format("    flagging:       {0:.6f}s", stats.flagging_time / iterations) << endl;
    cout << format("    vertex:         {0:.6f}s", stats.vertex_time / iterations) << endl;
    cout << format("    geometry:       {0:.6f}s", stats.geometry_time / iterations) << endl;
    cout <<        "----------------------------------------" << endl << endl;
}

float sphereFunc(Vector3 v)
{
    return mag(v - Vector3{ 4, 4, 4 }) / 4.0f;
}

int main()
{
    runBenchmark("sphere", 100, { 0, 0, 0 }, { 8, 8, 8 }, 0.08f, sphereFunc, 1.0f);
}
