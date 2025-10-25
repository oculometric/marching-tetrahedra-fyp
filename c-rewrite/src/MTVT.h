#pragma once

#include <vector>
#include <cstdint>

#include "Vector3.h"

struct MTVTMesh
{
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<uint16_t> indices;
};

struct MTVTDebugStats
{
    double allocation_time = 0;
    double sampling_time = 0;
    double vertex_time = 0;
    double geometry_time = 0;
    size_t sample_points_allocated = 0;
    size_t min_sample_points = 0;
    size_t edges_allocated = 0;
    size_t min_edges = 0;
    size_t tetrahedra = 0;
    size_t vertices = 0;
    size_t indices = 0;
    size_t cubes_x = 0;
    size_t cubes_y = 0;
    size_t cubes_z = 0;
    size_t degenerate_triangles = 0;
};

class MTVTBuilder
{
public:
    enum LatticeType
    {
        BODY_CENTERED_DIAMOND,
        SIMPLE_CUBIC
    };

    enum ClusteringMode
    {
        NONE,
        INTEGRATED,
        POST_PROCESED
    };

private:
    struct EdgeReferences
    {
        uint16_t references[14];
    };

private:
    float (*sampler)(Vector3);
    float threshold;
    Vector3 min_extent, max_extent;
    Vector3 size;
    int cubes_x, cubes_y, cubes_z;
    int samples_x, samples_y, samples_z;
    float resolution;
    size_t grid_data_length;

    LatticeType structure;
    ClusteringMode clustering;

    int index_offsets_evenz[14];
    int index_offsets_oddz[14];

    float* sample_values = nullptr;
    Vector3* sample_positions = nullptr;
    uint16_t* sample_crossing_flags = nullptr;
    EdgeReferences* sample_edge_indices = nullptr;
    std::vector<Vector3> vertices;
    std::vector<uint16_t> indices;
    size_t degenerate_triangles;
    size_t tetrahedra_evaluated;

public:
    MTVTBuilder();

    void configure(Vector3 minimum_extent, Vector3 maximum_extent, float cube_size, float (*sample_func)(Vector3), float threshold_value);
    void configureModes(LatticeType lattice_type, ClusteringMode clustering_mode);
    MTVTMesh generate(MTVTDebugStats& stats);

private:
    void prepareBuffers();
    void destroyBuffers();
    void populateIndexOffsets();
    void samplingPass();
    void vertexPass();
    void geometryPass();
};