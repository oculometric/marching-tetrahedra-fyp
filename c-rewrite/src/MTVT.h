#pragma once

#include <vector>

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
    double flagging_time = 0;
    double vertex_time = 0;
    double geometry_time = 0;
    size_t sample_points = 0;
    size_t edges = 0;
    size_t tetrahedra = 0;
    size_t vertices = 0;
    size_t indices = 0;
    int cubes_x = 0;
    int cubes_y = 0;
    int cubes_z = 0;
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

    bool cache_positions_enabled;
    LatticeType structure;
    ClusteringMode clustering;

    int index_offsets_evenz[14];
    int index_offsets_oddz[14];

    float* sample_values = nullptr;
    Vector3* sample_positions = nullptr;
    uint16_t* sample_proximity_flags = nullptr;
    EdgeReferences* sample_edge_indices = nullptr;
    std::vector<Vector3> vertices;
    std::vector<uint16_t> indices;

public:
    MTVTBuilder();

    void configure(Vector3 minimum_extent, Vector3 maximum_extent, float cube_size, float (*sample_func)(Vector3), float threshold_value);
    void configureModes(bool cache_positions, LatticeType lattice_type, ClusteringMode clustering_mode);
    MTVTMesh generate(MTVTDebugStats& stats);

private:
    void prepareBuffers();
    void destroyBuffers();
    void samplingPass();
    void flaggingPass();
    void vertexPass();
    void geometryPass();
};