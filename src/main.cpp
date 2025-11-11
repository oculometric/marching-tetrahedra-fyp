#include <iostream>
#include <string>
#include <format>
#include <fstream>
#include <chrono>

#include "MTVT.h"
#include "fbm.h"
#include "mesh_closest.h"
#include "mesh_stats.h"
#include "graphics.h"

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

string runBenchmark(string name, int iterations, Vector3 min, Vector3 max, float cube_size, float (*sampler)(Vector3), float threshold, Builder::LatticeType lattice_type, Builder::ClusteringMode clustering_mode, unsigned short threads)
{
    Builder builder;
    try
    {
        builder.configure(min, max, cube_size, sampler, threshold);
        builder.configureModes(lattice_type, clustering_mode, threads);
    }
    catch (exception e)
    {
        cout << "exception during " << name << " benchmark:" << endl;
        cout << e.what() << endl;
        return "";
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
            return "";
        }
    }

    size_t triangles = stats.indices / 3;
    double total_time = stats.allocation_time + stats.sampling_time + stats.vertex_time + stats.geometry_time;
    double mean_time = total_time / iterations;
    double mean_allocation = stats.allocation_time / iterations;
    double mean_sampling = stats.sampling_time / iterations;
    double mean_vertex = stats.vertex_time / iterations;
    double mean_geometry = stats.geometry_time / iterations;
    float allocation_time_pct = (stats.allocation_time / total_time) * 100.0f;
    float sampling_time_pct = (stats.sampling_time / total_time) * 100.0f;
    float vertex_time_pct = (stats.vertex_time / total_time) * 100.0f;
    float geometry_time_pct = (stats.geometry_time / total_time) * 100.0f;
    float sp_alloc_pct = ((float)stats.sample_points_allocated / stats.min_sample_points) * 100.0f;
    float e_alloc_pct = ((float)stats.edges_allocated / stats.min_edges) * 100.0f;
    float t_eval_pct = ((float)stats.tetrahedra_evaluated / stats.max_tetrahedra) * 100.0f;
    size_t vertex_bytes = sizeof(Vector3) * mesh.vertices.size();
    size_t triangle_bytes = sizeof(VertexRef) * mesh.indices.size();
    MeshStats triangle_stats = compileStats(mesh);
    float degenerate_pct = ((float)stats.degenerate_triangles / ((float)triangles + (float)stats.degenerate_triangles)) * 100.0f;
    float verts_per_sp = (float)stats.vertices / (float)stats.min_sample_points;
    float verts_per_edge = (float)stats.vertices / (float)stats.min_edges;
    float verts_per_tet = (float)stats.vertices / (float)stats.max_tetrahedra;
    float tris_per_sp = (float)triangles / (float)stats.min_sample_points;
    float tris_per_edge = (float)triangles / (float)stats.min_edges;
    float tris_per_tet = (float)triangles / (float)stats.max_tetrahedra;
    string lattice_str = (lattice_type == Builder::BODY_CENTERED_DIAMOND) ? "BCDL" : "SIMPLE";
    string clustering_str = (clustering_mode == Builder::NONE) ? "NONE" : "INTEGRATED";
    if (clustering_mode == Builder::POST_PROCESED) clustering_str = "POSTPROCESSED";

    cout << '\b';
    cout <<        "-- summary -----------------------------" << endl;
    cout << format("  {0} test ({1} iterations)", name, iterations) << endl;
    cout << format("  {0}x{1}x{2} resolution", stats.cubes_x, stats.cubes_y, stats.cubes_z) << endl;
    cout <<        "  results:" << endl;
    cout << format(locale("en_US.UTF-8"), "    sample points:  {0:>12L} ({1:L} allocated)", stats.min_sample_points, stats.sample_points_allocated) << endl;
    cout << format(locale("en_US.UTF-8"), "    edges:          {0:>12L} ({1:L} allocated)", stats.min_edges, stats.edges_allocated) << endl;
    cout << format(locale("en_US.UTF-8"), "    tetrahedra:     {0:>12L} ({1:L} evaluated)", stats.max_tetrahedra, stats.tetrahedra_evaluated) << endl;
    cout << format(locale("en_US.UTF-8"), "    vertices:       {0:>12L}", stats.vertices) << endl;
    cout << format(locale("en_US.UTF-8"), "    triangles:      {0:>12L} ({1:L} indices)", triangles, stats.indices) << endl;
    cout << format(locale("en_US.UTF-8"), "    degenerates:    {0:>12L}", stats.degenerate_triangles) << endl;
    cout << format(locale("en_US.UTF-8"), "    invalid:        {0:>12L}", stats.invalid_triangles) << endl;
    cout << format("  timing:           {0:.>6f}s total", mean_time) << endl;
    cout << format("    allocation:     {0:.>6f}s ({1:5f}% of total)", mean_allocation, allocation_time_pct) << endl;
    cout << format("    sampling:       {0:.>6f}s ({1:5f}% of total)", mean_sampling, sampling_time_pct) << endl;
    cout << format("    vertex:         {0:.>6f}s ({1:5f}% of total)", mean_vertex, vertex_time_pct) << endl;
    cout << format("    geometry:       {0:.>6f}s ({1:5f}% of total)", mean_geometry, geometry_time_pct) << endl;
    cout <<        "  efficiency (lower number better):" << endl;
    cout << format("    SP allocation:  {0:>6f}% ({1})", sp_alloc_pct, memorySize(stats.mem_sample_points)) << endl;
    cout << format("    E allocation:   {0:>6f}% ({1})", e_alloc_pct, memorySize(stats.mem_edges)) << endl;
    cout << format("    T evaluation:   {0:>6f}%", t_eval_pct) << endl;
    cout << format("    vertex buffer:  {0}", memorySize(vertex_bytes)) << endl;
    cout << format("    index buffer:   {0}", memorySize(triangle_bytes)) << endl;
    cout <<        "  geometry stats:" << endl;
    cout << format("    degenerate fraction:       {0:>6f}%", degenerate_pct) << endl;
    cout << format("    vertices per SP:           {0:>6f}", verts_per_sp) << endl;
    cout << format("    vertices per edge:         {0:>6f}", verts_per_edge) << endl;
    cout << format("    vertices per tetrahedron:  {0:>6f}", verts_per_tet) << endl;
    cout << format("    triangles per SP:          {0:>6f}", tris_per_sp) << endl;
    cout << format("    triangles per edge:        {0:>6f}", tris_per_edge) << endl;
    cout << format("    triangles per tetrahedron: {0:>6f}", tris_per_tet) << endl;
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

    ofstream file("out/" + name + ".obj");
    if (!file.is_open())
        return "";
    file << "# this file was generated by MTVT" << endl;
    for (Vector3 v : mesh.vertices)
        file << format("v {0:8f} {1:8f} {2:8f}\n", v.x, v.y, v.z);
    if (!mesh.indices.empty())
        for (size_t i = 0; i < mesh.indices.size() - 2; i += 3)
            file << format("f {0} {1} {2}\n", mesh.indices[i] + 1, mesh.indices[i + 1] + 1, mesh.indices[i + 2] + 1);
    file.close();

    // column format: test name, test resolution x, y, z, test iterations, lattice type, merge mode, threads,
    // sample points, edges, tetrahedra, vertices, triangles, indices, degenerates, 
    // theo SPs, theo edges, theo tetrahedra,
    // total time, allocation time, sampling time, vertex time, geometry time,
    // allocation %, sampling %, vertex %, geometry %,
    // sp alloc size, e alloc size, vertex buf size, index buf size,
    // sp alloc efficiency, e alloc efficiency, t eval efficiency,
    // degenerate fract, verts per sp, verts per e, verts per tet, tris per sp, tris per e, tris per tet,
    // tri area mean, max, min, sd, tri aspect mean, max, min, sd
    string csv_line =
        format("{0};{1};{2};{3};{4};{5};{6};{7};", name, stats.cubes_x, stats.cubes_y, stats.cubes_z, iterations, lattice_str, clustering_str, threads)
        + format("{0};{1};{2};{3};{4};{5};{6};", stats.sample_points_allocated, stats.edges_allocated, stats.tetrahedra_evaluated, stats.vertices, triangles, stats.indices, stats.degenerate_triangles)
        + format("{0};{1};{2};", stats.min_sample_points, stats.min_edges, stats.max_tetrahedra)
        + format("{0:.>6f};{1:.>6f};{2:.>6f};{3:.>6f};{4:.>6f};", mean_time, mean_allocation, mean_sampling, mean_vertex, mean_geometry)
        + format("{0:5f}%;{1:5f}%;{2:5f}%;{3:5f}%;", allocation_time_pct, sampling_time_pct, vertex_time_pct, geometry_time_pct)
        + format("{0};{1};{2};{3};", stats.mem_sample_points, stats.mem_edges, vertex_bytes, triangle_bytes)
        + format("{0:>6f}%;{1:>6f}%;{2:>6f}%;", sp_alloc_pct, e_alloc_pct, t_eval_pct)
        + format("{0:>6f}%;{1:>8f};{2:>8f};{3:>8f};{4:>8f};{5:>8f};{6:>8f};", degenerate_pct, verts_per_sp, verts_per_edge, verts_per_tet, tris_per_sp, tris_per_edge, tris_per_tet)
        + format("{0:>8f};{1:>8f};{2:>8f};{3:>8f};{4:>8f};{5:>8f};{6:>8f};{7:>8f}\n", triangle_stats.area_mean, triangle_stats.area_max, triangle_stats.area_min, triangle_stats.area_sd, triangle_stats.aspect_mean, triangle_stats.aspect_max, triangle_stats.aspect_min, triangle_stats.aspect_sd);
    return csv_line;
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
    GraphicsEnv graphics;
    Mesh temp_mesh = {
        { { -0.5f, -0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { 0.0f,  0.5f, 0.0f } },
        {},
        {}
    };
    graphics.create(1024, 1024);
    Builder builder;
    builder.configure({ -1, -1, -1 }, { 1, 1, 1 }, 0.2f, fbmFunc, 0.0f);
    builder.configureModes(Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    DebugStats stats;
    graphics.setMesh(builder.generate(stats));

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
    bunny_mesh.load("res/stanford_bunny/bunny_touchup.obj");

    runBenchmark("bunny", 1, { -0.1f, -0.06f, -0.01f }, { 0.1f, 0.08f, 0.16f }, 0.04f, [](Vector3 v) { return bunny_mesh.closestPointSDF(v); }, 0.0F, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    
    auto current_time = chrono::current_zone()->to_local(chrono::system_clock::now());
    string filename = format("out/benchmark_{0:%d_%m_%Y %H.%M.%S}.csv", current_time);
    ofstream csv(filename);
    if (!csv.is_open())
        return 1;
    csv << csv_file;
    csv.close();

    return 0;
}
