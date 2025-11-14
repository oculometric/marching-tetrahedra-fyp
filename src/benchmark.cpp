#include "benchmark.h"

#include <vector>
#include <iostream>
#include <format>
#include <fstream>

using namespace std;
using namespace MTVT;

TriangleStats MTVT::computeTriangleQualityStats(const Mesh& mesh)
{
    TriangleStats stats{ };

    double area_sum = 0;
    stats.area_max = 0;
    stats.area_min = 0;
    vector<double> areas;   areas.reserve(mesh.indices.size() / 3);
    double aspect_sum = 0;
    stats.aspect_max = 0;
    stats.aspect_min = 0;
    vector<double> aspects; aspects.reserve(mesh.indices.size() / 3);

    if (mesh.indices.size() == 0)
        return stats;

    for (size_t i = 0; i < mesh.indices.size() - 2; i += 3)
    {
        // compute area
        // compute aspect
        Vector3 v0 = mesh.vertices[mesh.indices[i]];
        Vector3 v1 = mesh.vertices[mesh.indices[i + 1]];
        Vector3 v2 = mesh.vertices[mesh.indices[i + 2]];
        
        Vector3 v01 = v1 - v0; double l01 = mag(v01);
        Vector3 v02 = v2 - v0; double l02 = mag(v02);

        double area = mag(v01 % v02) * 0.5f;
        double aspect = ::max(l01, l02) / ::min(l01, l02);
        area_sum += area;
        areas.push_back(area);
        if (area > stats.area_max) stats.area_max = area;
        if (area < stats.area_min) stats.area_min = area;
        aspect_sum += aspect;
        aspects.push_back(aspect);
        if (aspect > stats.aspect_max) stats.aspect_max = aspect;
        if (aspect < stats.aspect_min) stats.aspect_min = aspect;
    }

    size_t n = areas.size();
    stats.area_mean = area_sum / n;
    stats.aspect_mean = aspect_sum / n;

    double area_sq_dev_sum = 0;
    double aspect_sq_dev_sum = 0;

    for (size_t i = 0; i < n; i++)
    {
        double area_dev = (areas[i] - stats.area_mean);
        area_sq_dev_sum += area_dev * area_dev;
        double aspect_dev = (aspects[i] - stats.aspect_mean);
        aspect_sq_dev_sum += aspect_dev * aspect_dev;
    }

    stats.area_sd = sqrt(area_sq_dev_sum / n);
    stats.aspect_sd = sqrt(aspect_sq_dev_sum / n);

    return stats;
}

pair<SummaryStats, Mesh> MTVT::runBenchmark(std::string name, int iterations, Vector3 min, Vector3 max, float cube_size, float(*sampler)(Vector3), float threshold, Builder::LatticeType lattice_type, Builder::ClusteringMode clustering_mode, unsigned short threads)
{
    SummaryStats summary{ };

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
        return { summary, {} };
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
            return { summary, mesh };
        }
    }

    summary.name = name;
    summary.iterations = iterations;
    summary.threads = threads;

    summary.vertices = stats.vertices;
    summary.vertices_bytes = sizeof(Vector3) * mesh.vertices.size();
    summary.triangles = stats.indices / 3;
    summary.indices = stats.indices;
    summary.indices_bytes = sizeof(VertexRef) * mesh.indices.size();

    summary.cubes_x = stats.cubes_x;
    summary.cubes_y = stats.cubes_y;
    summary.cubes_z = stats.cubes_z;
    summary.lattice_type = (lattice_type == Builder::BODY_CENTERED_DIAMOND) ? "BCDL" : "SIMPLE";
    summary.clustering_mode = (clustering_mode == Builder::NONE) ? "NONE" : "INTEGRATED";
    if (clustering_mode == Builder::POST_PROCESED) summary.clustering_mode = "POSTPROCESSED";
    summary.sample_points_allocated = stats.sample_points_allocated;
    summary.sample_points_theoretical = stats.min_sample_points;
    summary.sample_points_bytes = stats.mem_sample_points;
    summary.sample_points_allocated_percent = ((float)stats.sample_points_allocated / (float)stats.min_sample_points) * 100.0f;
    summary.edges_allocated = stats.edges_allocated;
    summary.edges_theoretical = stats.min_edges;
    summary.edges_bytes = stats.mem_edges;
    summary.edges_allocated_percent = ((float)stats.edges_allocated / (float)stats.min_edges) * 100.0f;
    summary.tetrahedra_computed = stats.tetrahedra_evaluated;
    summary.tetrahedra_total = stats.max_tetrahedra;
    summary.tetrahedra_computed_percent = ((float)stats.tetrahedra_evaluated / (float)stats.max_tetrahedra) * 100.0f;

    double total_time = stats.allocation_time + stats.sampling_time + stats.vertex_time + stats.geometry_time;
    summary.time_total = total_time / iterations;
    summary.time_allocation = stats.allocation_time / iterations;
    summary.percent_allocation = (stats.allocation_time / total_time) * 100.0f;
    summary.time_sampling = stats.sampling_time / iterations;
    summary.percent_sampling = (stats.sampling_time / total_time) * 100.0f;
    summary.time_vertex = stats.vertex_time / iterations;
    summary.percent_vertex = (stats.vertex_time / total_time) * 100.0f;
    summary.time_geometry = stats.geometry_time / iterations;
    summary.percent_geometry = (stats.geometry_time / total_time) * 100.0f;

    summary.degenerate_triangles = stats.degenerate_triangles;
    summary.degenerate_percent = ((float)stats.degenerate_triangles / ((float)summary.triangles + (float)stats.degenerate_triangles)) * 100.0f;
    summary.invalid_triangles = stats.invalid_triangles;
    summary.verts_per_sp = (float)stats.vertices / (float)stats.min_sample_points;
    summary.verts_per_edge = (float)stats.vertices / (float)stats.min_edges;
    summary.verts_per_tet = (float)stats.vertices / (float)stats.max_tetrahedra;
    summary.tris_per_sp = (float)summary.triangles / (float)stats.min_sample_points;
    summary.tris_per_edge = (float)summary.triangles / (float)stats.min_edges;
    summary.tris_per_tet = (float)summary.triangles / (float)stats.max_tetrahedra;
    summary.triangle_stats = computeTriangleQualityStats(mesh);

    return { summary, mesh };
}

string MTVT::generateCSVLine(const SummaryStats& stats)
{
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
        format("{0};{1};{2};{3};{4};{5};{6};{7};", stats.name, stats.cubes_x, stats.cubes_y, stats.cubes_z, stats.iterations, stats.lattice_type, stats.clustering_mode, stats.threads)
        + format("{0};{1};{2};{3};{4};{5};{6};", stats.sample_points_allocated, stats.edges_allocated, stats.tetrahedra_computed, stats.vertices, stats.triangles, stats.indices, stats.degenerate_triangles)
        + format("{0};{1};{2};", stats.sample_points_theoretical, stats.edges_theoretical, stats.tetrahedra_total)
        + format("{0:.>6f};{1:.>6f};{2:.>6f};{3:.>6f};{4:.>6f};", stats.time_total, stats.time_allocation, stats.time_sampling, stats.time_vertex, stats.time_geometry)
        + format("{0:5f}%;{1:5f}%;{2:5f}%;{3:5f}%;", stats.percent_allocation, stats.percent_sampling, stats.percent_vertex, stats.percent_geometry)
        + format("{0};{1};{2};{3};", stats.sample_points_bytes, stats.edges_bytes, stats.vertices_bytes, stats.indices_bytes)
        + format("{0:>6f}%;{1:>6f}%;{2:>6f}%;", stats.sample_points_allocated_percent, stats.edges_allocated_percent, stats.tetrahedra_computed_percent)
        + format("{0:>6f}%;{1:>8f};{2:>8f};{3:>8f};{4:>8f};{5:>8f};{6:>8f};", stats.degenerate_percent, stats.verts_per_sp, stats.verts_per_edge, stats.verts_per_tet, stats.tris_per_sp, stats.tris_per_edge, stats.tris_per_tet)
        + format("{0:>8f};{1:>8f};{2:>8f};{3:>8f};{4:>8f};{5:>8f};{6:>8f};{7:>8f}\n", stats.triangle_stats.area_mean, stats.triangle_stats.area_max, stats.triangle_stats.area_min, stats.triangle_stats.area_sd, stats.triangle_stats.aspect_mean, stats.triangle_stats.aspect_max, stats.triangle_stats.aspect_min, stats.triangle_stats.aspect_sd);
    return csv_line;
}

void MTVT::printBenchmarkSummary(const SummaryStats& stats)
{
    cout <<        "-- summary -----------------------------" << endl;
    cout << format("  {0} test ({1} iterations)", stats.name, stats.iterations) << endl;
    cout << format("  {0}x{1}x{2} resolution", stats.cubes_x, stats.cubes_y, stats.cubes_z) << endl;
    cout <<        "  results:" << endl;
    cout << format(locale("en_US.UTF-8"), "    sample points:  {0:>12L} ({1:L} allocated)", stats.sample_points_theoretical, stats.sample_points_allocated) << endl;
    cout << format(locale("en_US.UTF-8"), "    edges:          {0:>12L} ({1:L} allocated)", stats.edges_theoretical, stats.edges_allocated) << endl;
    cout << format(locale("en_US.UTF-8"), "    tetrahedra:     {0:>12L} ({1:L} evaluated)", stats.tetrahedra_total, stats.tetrahedra_computed) << endl;
    cout << format(locale("en_US.UTF-8"), "    vertices:       {0:>12L}", stats.vertices) << endl;
    cout << format(locale("en_US.UTF-8"), "    triangles:      {0:>12L} ({1:L} indices)", stats.triangles, stats.indices) << endl;
    cout << format(locale("en_US.UTF-8"), "    degenerates:    {0:>12L}", stats.degenerate_triangles) << endl;
    cout << format(locale("en_US.UTF-8"), "    invalid:        {0:>12L}", stats.invalid_triangles) << endl;
    cout << format("  timing:           {0:.>6f}s total", stats.time_total) << endl;
    cout << format("    allocation:     {0:.>6f}s ({1:5f}% of total)", stats.time_allocation, stats.percent_allocation) << endl;
    cout << format("    sampling:       {0:.>6f}s ({1:5f}% of total)", stats.time_sampling, stats.percent_sampling) << endl;
    cout << format("    vertex:         {0:.>6f}s ({1:5f}% of total)", stats.time_vertex, stats.percent_vertex) << endl;
    cout << format("    geometry:       {0:.>6f}s ({1:5f}% of total)", stats.time_geometry, stats.percent_geometry) << endl;
    cout <<        "  efficiency (lower number better):" << endl;
    cout << format("    SP allocation:  {0:>6f}% ({1})", stats.sample_points_allocated_percent, getMemorySize(stats.sample_points_bytes)) << endl;
    cout << format("    E allocation:   {0:>6f}% ({1})", stats.edges_allocated_percent, getMemorySize(stats.edges_bytes)) << endl;
    cout << format("    T evaluation:   {0:>6f}%", stats.tetrahedra_computed_percent) << endl;
    cout << format("    vertex buffer:  {0}", getMemorySize(stats.vertices_bytes)) << endl;
    cout << format("    index buffer:   {0}", getMemorySize(stats.indices_bytes)) << endl;
    cout <<        "  geometry stats:" << endl;
    cout << format("    degenerate fraction:       {0:>6f}%", stats.degenerate_percent) << endl;
    cout << format("    vertices per SP:           {0:>6f}", stats.verts_per_sp) << endl;
    cout << format("    vertices per edge:         {0:>6f}", stats.verts_per_edge) << endl;
    cout << format("    vertices per tetrahedron:  {0:>6f}", stats.verts_per_tet) << endl;
    cout << format("    triangles per SP:          {0:>6f}", stats.tris_per_sp) << endl;
    cout << format("    triangles per edge:        {0:>6f}", stats.tris_per_edge) << endl;
    cout << format("    triangles per tetrahedron: {0:>6f}", stats.tris_per_tet) << endl;
    cout <<        "  triangle quality stats:" << endl;
    cout << format("    area mean:     {0:>6f}", stats.triangle_stats.area_mean) << endl;
    cout << format("    area max:      {0:>6f}", stats.triangle_stats.area_max) << endl;
    cout << format("    area min:      {0:>6f}", stats.triangle_stats.area_min) << endl;
    cout << format("    area StdDev:   {0:>6f}", stats.triangle_stats.area_sd) << endl;
    cout << format("    aspect mean:   {0:>6f}", stats.triangle_stats.aspect_mean) << endl;
    cout << format("    aspect max:    {0:>6f}", stats.triangle_stats.aspect_max) << endl;
    cout << format("    aspect min:    {0:>6f}", stats.triangle_stats.aspect_min) << endl;
    cout << format("    aspect StdDev: {0:>6f}", stats.triangle_stats.aspect_sd) << endl;
    cout <<        "----------------------------------------" << endl << endl;
}

void MTVT::dumpMeshToOBJ(const Mesh& mesh, std::string name)
{
    ofstream file("out/" + name + ".obj");
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

string MTVT::getMemorySize(size_t bytes)
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