#pragma once

#include <vector>
#include <cstdint>

#include "Vector3.h"

namespace MTVT
{

//#define DEBUG_GRID

typedef uint32_t VertexRef;
typedef uint16_t EdgeFlags;
typedef size_t Index;
typedef uint8_t EdgeAddr;

struct Mesh
{
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<VertexRef> indices;
};

struct DebugStats
{
    double allocation_time = 0;
    double sampling_time = 0;
    double vertex_time = 0;
    double geometry_time = 0;
    size_t sample_points_allocated = 0;
    size_t min_sample_points = 0;
    size_t mem_sample_points = 0;
    size_t edges_allocated = 0;
    size_t min_edges = 0;
    size_t mem_edges = 0;
    size_t tetrahedra_evaluated = 0;
    size_t max_tetrahedra = 0;
    size_t vertices = 0;
    size_t indices = 0;
    size_t cubes_x = 0;
    size_t cubes_y = 0;
    size_t cubes_z = 0;
    size_t degenerate_triangles = 0;
    size_t invalid_triangles = 0;
};

class Builder
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
        VertexRef references[14];
    };

private:
    float (*sampler)(Vector3);
    float threshold;
    Vector3 min_extent, max_extent;
    Vector3 size;
    int cubes_x, cubes_y, cubes_z;
    int samples_x, samples_y, samples_z;
    float resolution;
    unsigned short thread_count;
    size_t grid_data_length;

    LatticeType structure;
    ClusteringMode clustering;

    int index_offsets_evenz[14];
    int index_offsets_oddz[14];
    Vector3 vector_offsets[14];

    float* sample_values = nullptr;
#if defined DEBUG_GRID
    Vector3* sample_positions = nullptr;
#endif
    EdgeFlags* sample_crossing_flags = nullptr;
    EdgeReferences* sample_edge_indices = nullptr;
    std::vector<Vector3> vertices;
    std::vector<VertexRef> indices;
    size_t degenerate_triangles;
    size_t invalid_triangles;
    size_t tetrahedra_evaluated;

public:
    Builder();

    void configure(Vector3 minimum_extent, Vector3 maximum_extent, float cube_size, float (*sample_func)(Vector3), float threshold_value);
    void configureModes(LatticeType lattice_type, ClusteringMode clustering_mode, unsigned short parallel_threads);
    Mesh generate(DebugStats& stats);

private:
    void prepareBuffers();
    void destroyBuffers();
    void populateIndexOffsets();
    void samplingPass();
    void samplingLayer(const int start, const int layers);
    VertexRef addVertex(const float* neighbour_values, const EdgeAddr p, const float thresh_diff, const float value, const Vector3& position, std::vector<Vector3>& verts);
    VertexRef addMergedVertex(const float* neighbour_values, const EdgeAddr p, const float thresh_diff, const float value, const Vector3& position, bool* usable_neighbours, std::vector<Vector3>& verts, EdgeReferences& edge_refs);
    void addVerticesIndividually(const float* neighbour_values, const float thresh_diff, const float value, const Vector3& position, EdgeFlags usable_edges, std::vector<Vector3>& verts, EdgeReferences& edge_refs);
    void vertexPass();
    void geometryPass();
};

}