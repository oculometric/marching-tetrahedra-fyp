#include <iostream>
#include <string>
#include <format>
#include <fstream>

#include "MTVT.h"
#include "fbm.h"
#include "mesh_closest.h"
#include "mesh_stats.h"

using namespace std;
using namespace MTVT;

static string memorySize(size_t bytes)
{
    if (bytes >= 2048ull * 1024 * 1024)
        return format("{0:.1f} GiB", (float)bytes / (1024ull * 1024 * 1024));
    else if (bytes >= 2048ull * 1024)
        return format("{0:.1f} MiB", (float)bytes / (1024ull * 1024));
    else if (bytes >= 2048ull)
        return format("{0:.1f} KiB", (float)bytes / 1024);
    else
        return format("{0} B", bytes);
}

void runBenchmark(string name, int iterations, Vector3 min, Vector3 max, float cube_size, float (*sampler)(Vector3), float threshold)
{
    Builder builder;
    try
    {
        builder.configure(min, max, cube_size, sampler, threshold);
        builder.configureModes(Builder::LatticeType::BODY_CENTERED_DIAMOND, Builder::ClusteringMode::INTEGRATED, 8);
    }
    catch (exception e)
    {
        cout << "exception during " << name << " benchmark:" << endl;
        cout << e.what() << endl;
        return;
    }
    DebugStats stats;
    Mesh mesh;

    for (int i = 0; i < iterations; ++i)
    {
        cout << format("{0} test iteration {1}/{2}\r", name, i, iterations);
        cout.flush();
        try
        {
            mesh = builder.generate(stats);
        }
        catch (exception e)
        {
            cout << "exception during " << name << " benchmark:" << endl;
            cout << e.what() << endl;
            return;
        }
    }

    float total_time = stats.allocation_time + stats.sampling_time + stats.vertex_time + stats.geometry_time;
    MeshStats triangle_stats = compileStats(mesh);

    cout << '\b';
    cout <<        "-- summary -----------------------------" << endl;
    cout << format("  {0} test ({1} iterations)", name, iterations) << endl;
    cout << format("  {0}x{1}x{2} resolution", stats.cubes_x, stats.cubes_y, stats.cubes_z) << endl;
    cout <<        "  results:" << endl;
    cout << format(locale("en_US.UTF-8"), "    sample points:  {0:>12L} ({1:L} allocated)", stats.min_sample_points, stats.sample_points_allocated) << endl;
    cout << format(locale("en_US.UTF-8"), "    edges:          {0:>12L} ({1:L} allocated)", stats.min_edges, stats.edges_allocated) << endl;
    cout << format(locale("en_US.UTF-8"), "    tetrahedra:     {0:>12L} ({1:L} evaluated)", stats.max_tetrahedra, stats.tetrahedra_evaluated) << endl;
    cout << format(locale("en_US.UTF-8"), "    vertices:       {0:>12L}", stats.vertices) << endl;
    cout << format(locale("en_US.UTF-8"), "    triangles:      {0:>12L} ({1:L} indices)", stats.indices / 3, stats.indices) << endl;
    cout << format(locale("en_US.UTF-8"), "    degenerates:    {0:>12L}", stats.degenerate_triangles) << endl;
    cout << format("  timing:           {0:.>6f}s total", total_time / iterations) << endl;
    cout << format("    allocation:     {0:.>6f}s ({1:5f}% of total)", stats.allocation_time / iterations, (stats.allocation_time / total_time) * 100.0f) << endl;
    cout << format("    sampling:       {0:.>6f}s ({1:5f}% of total)", stats.sampling_time / iterations, (stats.sampling_time / total_time) * 100.0f) << endl;
    cout << format("    vertex:         {0:.>6f}s ({1:5f}% of total)", stats.vertex_time / iterations, (stats.vertex_time / total_time) * 100.0f) << endl;
    cout << format("    geometry:       {0:.>6f}s ({1:5f}% of total)", stats.geometry_time / iterations, (stats.geometry_time / total_time) * 100.0f) << endl;
    cout <<        "  efficiency (lower number better):" << endl;
    cout << format("    SP allocation:  {0:>6f}% ({1})", ((float)stats.sample_points_allocated / stats.min_sample_points) * 100.0f, memorySize(stats.mem_sample_points)) << endl;
    cout << format("    E allocation:   {0:>6f}% ({1})", ((float)stats.edges_allocated / stats.min_edges) * 100.0f, memorySize(stats.mem_edges)) << endl;
    cout << format("    T evaluation:   {0:>6f}%", ((float)stats.tetrahedra_evaluated / stats.max_tetrahedra) * 100.0f) << endl;
    cout <<        "  geometry stats:" << endl;
    cout << format("    degenerate fraction:       {0:>6f}%", ((float)stats.degenerate_triangles / ((float)(stats.indices / 3) + (float)stats.degenerate_triangles)) * 100.0f) << endl;
    cout << format("    vertices per SP:           {0:>6f}", (float)stats.vertices / (float)stats.min_sample_points) << endl;
    cout << format("    vertices per edge:         {0:>6f}", (float)stats.vertices / (float)stats.min_edges) << endl;
    cout << format("    vertices per tetrahedron:  {0:>6f}", (float)stats.vertices / (float)stats.max_tetrahedra) << endl;
    cout << format("    triangles per SP:          {0:>6f}", (float)(stats.indices / 3) / (float)stats.min_sample_points) << endl;
    cout << format("    triangles per tetrahedron: {0:>6f}", (float)(stats.indices / 3) / (float)stats.min_edges) << endl;
    cout << format("    triangles per edge:        {0:>6f}", (float)(stats.indices / 3) / (float)stats.max_tetrahedra) << endl;
    cout <<        "  triangle quality stats:" << endl;
    cout << format("    area mean:     {0:>6f}", triangle_stats.area_mean) << endl;
    cout << format("    area max:      {0:>6f}", triangle_stats.area_max) << endl;
    cout << format("    area min:      {0:>6f}", triangle_stats.area_min) << endl;
    cout << format("    area StdDev:   {0:>6f}", triangle_stats.area_sd) << endl;
    cout << format("    aspect mean:   {0:>6f}", triangle_stats.aspect_mean) << endl;
    cout << format("    aspect max:    {0:>6f}", triangle_stats.aspect_max) << endl;
    cout << format("    aspect min:    {0:>6f}", triangle_stats.aspect_min) << endl;
    cout << format("    aspect StdDev: {0:>6f}", triangle_stats.aspect_sd) << endl;
    cout <<        "----------------------------------------" << endl << endl;

    ofstream file(name + ".obj");
    if (!file.is_open())
        return;
    file << "# this file was generated by MTVT" << endl;
    for (Vector3 v : mesh.vertices)
        file << format("v {0:8f} {1:8f} {2:8f}\n", v.x, v.y, v.z);
    if (!mesh.indices.empty())
        for (size_t i = 0; i < mesh.indices.size() - 2; i += 3)
            file << format("f {0} {1} {2}\n", mesh.indices[i] + 1, mesh.indices[i + 1] + 1, mesh.indices[i + 2] + 1);
    file.close();
}

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
    runBenchmark("sphere", 1, { -2, -2, -2 }, { 2, 2, 2 }, 0.04f, sphereFunc, 1.0f);
    runBenchmark("fbm", 1, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f);
    runBenchmark("bump", 1, { -4, -4, -4 }, { 4, 4, 4 }, 0.08f, [](Vector3 v) { return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z; }, 0.0f);
    
    bunny_mesh.load("../stanford_bunny/bunny_touchup.obj");

    //runBenchmark("bunny", 1, { -0.1f, -0.06f, -0.01f }, { 0.1f, 0.08f, 0.16f }, 0.004f, [](Vector3 v) { return bunny_mesh.closestPointSDF(v); }, 0.0f);

    return 0;
}
