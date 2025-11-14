#pragma once

#include <string>

#include "MTVT.h"

namespace MTVT
{

struct TriangleStats
{
    double area_mean;
    double area_max;
    double area_min;
    double area_sd;

    double aspect_mean;
    double aspect_max;
    double aspect_min;
    double aspect_sd;
};

struct SummaryStats
{
    // benchmark info
    std::string name;
    int iterations;
    unsigned short threads;

    // mesh stats
    size_t vertices, vertices_bytes;
    size_t triangles, indices, indices_bytes;

    // generation parameters/stats
    size_t cubes_x, cubes_y, cubes_z;
    std::string lattice_type, clustering_mode;
    size_t sample_points_allocated, sample_points_theoretical, sample_points_bytes;
    float sample_points_allocated_percent;
    size_t edges_allocated, edges_theoretical, edges_bytes;
    float edges_allocated_percent;
    size_t tetrahedra_computed, tetrahedra_total;
    float tetrahedra_computed_percent;

    // timing stats
    double time_total;
    double time_allocation, percent_allocation;
    double time_sampling, percent_sampling;
    double time_vertex, percent_vertex;
    double time_geometry, percent_geometry;
    double time_normals, percent_normals;

    // geometry stats
    size_t degenerate_triangles;
    float degenerate_percent;
    size_t invalid_triangles;
    float verts_per_sp, verts_per_edge, verts_per_tet;
    float tris_per_sp, tris_per_edge, tris_per_tet;
    TriangleStats triangle_stats;
};

TriangleStats computeTriangleQualityStats(const Mesh& mesh);
std::pair<SummaryStats, Mesh> runBenchmark(std::string name, int iterations, Vector3 min, Vector3 max, float cube_size, float (*sampler)(Vector3), float threshold, Builder::LatticeType lattice_type, Builder::ClusteringMode clustering_mode, unsigned short threads);
std::string generateCSVLine(const SummaryStats& stats, bool title_line = false);
void printBenchmarkSummary(const SummaryStats& stats);
std::string getMemorySize(size_t bytes);
void dumpMeshToOBJ(const Mesh& mesh, std::string name);

}