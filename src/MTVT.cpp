#include "MTVT.h"

#include <chrono>
#include <fstream>
#include <format>
#include <thread>
#include <intrin.h>

#define VERTEX_NULL (VertexRef)-1
#define INDEX_NULL (Index)-1
#define EDGE_NULL (EdgeAddr)-1

using namespace std;
using namespace MTVT;

static inline size_t computeCubicFunction(size_t x, size_t y, size_t z, size_t a, size_t b, size_t c, size_t d)
{
    return (a * x * y * z) + (b * ((x * y) + (x * z) + (y * z))) + (c * (x + y + z)) + d;
}

Builder::Builder()
{
	configure({ -1, -1, -1 }, { 1, 1, 1 }, 1.0f, [](Vector3 v) -> float { return mag(v); }, 1.0f);
	configureModes(LatticeType::BODY_CENTERED_DIAMOND, ClusteringMode::NONE, 1);
}

void Builder::configure(Vector3 minimum_extent, Vector3 maximum_extent, float cube_size, float(*sample_func)(Vector3), float threshold_value)
{
    if (cube_size <= 0.0f)
        throw exception("mesh builder: invalid cube size");

    sampler = sample_func;
    threshold = threshold_value;
    resolution = ::abs(cube_size);
    min_extent = min(minimum_extent, maximum_extent);
    max_extent = max(minimum_extent, maximum_extent);
    size = max_extent - min_extent;
    float tmp = ceilf(size.x / resolution);
    if (tmp >= INT_MAX - 2) throw exception("mesh builder: sample volume X size too big");
    if (tmp < 1) throw exception("mesh builder: sample volume X size too small");
    cubes_x = (int)tmp;
    tmp = ceilf(size.y / resolution);
    if (tmp >= INT_MAX - 2) throw exception("mesh builder: sample volume Y size too big");
    if (tmp < 1) throw exception("mesh builder: sample volume X size too small");
    cubes_y = (int)tmp;
    tmp = ceilf(size.z / resolution);
    if (tmp >= (INT_MAX / 2) - 3) throw exception("mesh builder: sample volume Z size too big");
    if (tmp < 1) throw exception("mesh builder: sample volume X size too small");
    cubes_z = (int)tmp;
    // sample points will contain space for the entire lattice
    // this means we need space for cubes_s + 1 + cubes_s + 2 points for the BCDL
    // and every other layer in each direction is one sample shorter (and we just leave the last one blank)
    samples_x = cubes_x + 2;
    samples_y = cubes_y + 2;
    samples_z = (cubes_z * 2) + 3;
    grid_data_length = static_cast<size_t>(samples_x) * static_cast<size_t>(samples_y) * static_cast<size_t>(samples_z);
    if ((grid_data_length / static_cast<size_t>(samples_x)) / static_cast<size_t>(samples_y) != static_cast<size_t>(samples_z))
        throw exception("mesh builder: sample volume dimensions too big");
    int s_z = samples_x * samples_y * 2;
    if ((s_z / samples_x) / samples_y != 2)
        throw exception("mesh builder: sample volume X/Y size too big");
    populateIndexOffsets();
}

void Builder::configureModes(LatticeType lattice_type, ClusteringMode clustering_mode, unsigned short parallel_threads)
{
    structure = lattice_type;
    clustering = clustering_mode;
    thread_count = ::max((unsigned short)1, parallel_threads);
}

Mesh Builder::generate(DebugStats& stats)
{
    if (sampler == nullptr)
        return Mesh();

    auto allocation_start = chrono::high_resolution_clock::now();
    degenerate_triangles = 0;
    invalid_triangles = 0;
    tetrahedra_evaluated = 0;
    prepareBuffers();
    vertices.clear();
    indices.clear();
    float allocation = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - allocation_start)).count();

    auto sampling_start = chrono::high_resolution_clock::now();
    samplingPass();
    float sampling = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - sampling_start)).count();

    auto vertex_start = chrono::high_resolution_clock::now();
    vertexPass();
    float vertex = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - vertex_start)).count();

    VertexRef vert_max = VERTEX_NULL;
    if (vertices.size() >= (size_t)vert_max)
    {
        destroyBuffers();
        vertices.clear();
        throw exception("mesh builder: too many vertices generated, aborting");
    }

    auto geometry_start = chrono::high_resolution_clock::now();
    geometryPass();
    float geometry = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - geometry_start)).count();

    auto normaling_start = chrono::high_resolution_clock::now();
    computeVertexNormals();
    float normaling = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - normaling_start)).count();

    stats.allocation_time          += allocation;
    stats.sampling_time            += sampling;
    stats.vertex_time              += vertex;
    stats.geometry_time            += geometry;
    stats.normal_time              += normaling;
    stats.sample_points_allocated   = grid_data_length;
    stats.cubes_x                   = cubes_x;
    stats.cubes_y                   = cubes_y;
    stats.cubes_z                   = cubes_z;
    stats.min_sample_points         = computeCubicFunction(stats.cubes_x, stats.cubes_y, stats.cubes_z, 2, 3, 1, 1);
    stats.mem_sample_points         = sizeof(float) * grid_data_length;
#if defined DEBUG_GRID
    stats.mem_sample_points        += sizeof(Vector3) * grid_data_length;
#endif
    stats.edges_allocated           = grid_data_length * 14;
    stats.min_edges                 = computeCubicFunction(stats.cubes_x, stats.cubes_y, stats.cubes_z, 14, 11, 1, 0);
    stats.mem_edges                 = (sizeof(EdgeFlags) + sizeof(EdgeReferences)) * grid_data_length;
    stats.tetrahedra_evaluated      = tetrahedra_evaluated;
    stats.max_tetrahedra            = computeCubicFunction(stats.cubes_x, stats.cubes_y, stats.cubes_z, 12, 4, 0, 0);
    stats.vertices                  = vertices.size();
    stats.indices                   = indices.size();
    stats.degenerate_triangles      = degenerate_triangles;
    stats.invalid_triangles         = invalid_triangles;

    // DEBUG ZONE
#if defined DEBUG_GRID
    ofstream file("points.obj");
    file << "# this file was generated by MTVT" << endl;
    for (int i = 0; i < grid_data_length; i++)
    {
        Vector3 v = sample_positions[i];
        file << format("v {0:8f} {1:8f} {2:8f}\n", v.x, v.y, v.z);
    }

    file.close();
#endif

    destroyBuffers();
    return Mesh{ vertices, normals, indices };
}

void Builder::prepareBuffers()
{
    sample_values = new float[grid_data_length];
#if defined DEBUG_GRID
    sample_positions = new Vector3[grid_data_length];
#endif
    sample_crossing_flags = new EdgeFlags[grid_data_length];
    sample_edge_indices = new EdgeReferences[grid_data_length];
}

void Builder::destroyBuffers()
{
    delete[] sample_values; sample_values = nullptr;
#if defined DEBUG_GRID
    delete[] sample_positions; sample_positions = nullptr;
#endif
    delete[] sample_crossing_flags; sample_crossing_flags = nullptr;
    delete[] sample_edge_indices; sample_edge_indices = nullptr;
}

// these are all the possible EdgeAddr values, the defines give them
// readable names. specific to the diamond lattice pattern!
#define PX 0
#define NX 1
#define PY 2
#define NY 3
#define PZ 4
#define NZ 5

#define PXPYPZ 6
#define NXPYPZ 7
#define PXNYPZ 8
#define NXNYPZ 9
#define PXPYNZ 10
#define NXPYNZ 11
#define PXNYNZ 12
#define NXNYNZ 13

#define IS_POSITIVE_X(p) ((p == PX) || ((p >= 6) && ((p % 2) == 0)))
#define IS_NEGATIVE_X(p) ((p == NX) || ((p >= 6) && ((p % 2) == 1)))

void Builder::populateIndexOffsets()
{
    // generate a set of index offsets for surrounding sample points,
    // used in the flagging pass
    // the bit-flagging is as follows:
    int s_y = samples_x;
    int s_hz = samples_y * s_y;
    int s_z = s_hz * 2;

    index_offsets_evenz[PX] = 1;                    //  0 - 0b00000000 00000001 - +X
    index_offsets_evenz[NX] = -1;                   //  1 - 0b00000000 00000010 - -X
    index_offsets_evenz[PY] = s_y;                  //  2 - 0b00000000 00000100 - +Y
    index_offsets_evenz[NY] = -s_y;                 //  3 - 0b00000000 00001000 - -Y
    index_offsets_evenz[PZ] = s_z;                  //  4 - 0b00000000 00010000 - +Z
    index_offsets_evenz[NZ] = -s_z;                 //  5 - 0b00000000 00100000 - -Z

    index_offsets_evenz[PXPYPZ] = s_hz;             //  6 - 0b00000000 01000000 - +X+Y+Z
    index_offsets_evenz[NXPYPZ] = s_hz - 1;         //  7 - 0b00000000 10000000 - -X+Y+Z
    index_offsets_evenz[PXNYPZ] = s_hz - s_y;       //  8 - 0b00000001 00000000 - +X-Y+Z
    index_offsets_evenz[NXNYPZ] = s_hz - s_y - 1;   //  9 - 0b00000010 00000000 - -X-Y+Z
    index_offsets_evenz[PXPYNZ] = -s_hz;            // 10 - 0b00000100 00000000 - +X+Y-Z
    index_offsets_evenz[NXPYNZ] = -s_hz - 1;        // 11 - 0b00001000 00000000 - -X+Y-Z
    index_offsets_evenz[PXNYNZ] = -s_hz - s_y;      // 12 - 0b00010000 00000000 - +X-Y-Z
    index_offsets_evenz[NXNYNZ] = -s_hz - s_y - 1;  // 13 - 0b00100000 00000000 - -X-Y-Z

    index_offsets_oddz[PX] = index_offsets_evenz[PX];
    index_offsets_oddz[NX] = index_offsets_evenz[NX];
    index_offsets_oddz[PY] = index_offsets_evenz[PY];
    index_offsets_oddz[NY] = index_offsets_evenz[NY];
    index_offsets_oddz[PZ] = index_offsets_evenz[PZ];
    index_offsets_oddz[NZ] = index_offsets_evenz[NZ];

    index_offsets_oddz[PXPYPZ] = s_hz + s_y + 1;
    index_offsets_oddz[NXPYPZ] = s_hz + s_y;
    index_offsets_oddz[PXNYPZ] = s_hz + 1;
    index_offsets_oddz[NXNYPZ] = s_hz;
    index_offsets_oddz[PXPYNZ] = -s_hz + s_y + 1;
    index_offsets_oddz[NXPYNZ] = -s_hz + s_y;
    index_offsets_oddz[PXNYNZ] = -s_hz + 1;
    index_offsets_oddz[NXNYNZ] = -s_hz;

    float step = resolution;
    float diag = step / 2;

    vector_offsets[PX] = {  step, 0, 0 };
    vector_offsets[NX] = { -step, 0, 0 };
    vector_offsets[PY] = { 0,  step, 0 };
    vector_offsets[NY] = { 0, -step, 0 };
    vector_offsets[PZ] = { 0, 0,  step };
    vector_offsets[NZ] = { 0, 0, -step };

    vector_offsets[PXPYPZ] = {  diag,  diag,  diag };
    vector_offsets[NXPYPZ] = { -diag,  diag,  diag };
    vector_offsets[PXNYPZ] = {  diag, -diag,  diag };
    vector_offsets[NXNYPZ] = { -diag, -diag,  diag };
    vector_offsets[PXPYNZ] = {  diag,  diag, -diag };
    vector_offsets[NXPYNZ] = { -diag,  diag, -diag };
    vector_offsets[PXNYNZ] = {  diag, -diag, -diag };
    vector_offsets[NXNYNZ] = { -diag, -diag, -diag };
}

void Builder::samplingPass()
{
    int layers_each = samples_z / thread_count;
    int remainder = samples_z - (layers_each * thread_count);
    vector<thread*> threads;
    for (int i = 0; i < thread_count - 1; ++i)
        threads.push_back(new thread(&Builder::samplingLayer, this, layers_each * i, layers_each));
    samplingLayer(layers_each * (thread_count - 1), layers_each + remainder);
    for (thread* t : threads)
        t->join();
}

void MTVT::Builder::samplingLayer(const int start, const int layers)
{
    // sampling pass - compute the values at all of the sample points
    const float step = resolution / 2.0f;

    // our position in the array, saves recomputing this all the time
    Index index = static_cast<Index>(start) * samples_x * samples_y;
    // current sample point position
    Vector3 position = Vector3{ 0, 0, (min_extent.z - step) + (static_cast<Index>(start) * step) };
    for (int zi = start; zi < layers + start; ++zi)
    {
        // reset the Y position according to whether this is a key row (zi % 2 = 1)
        // or an off row (zi % 2 = 0). this creates the diamond pattern
        position.y = (zi % 2 == 0) ? (min_extent.y - step) : min_extent.y;
        for (int yi = 0; yi < samples_y; ++yi)
        {
            // similarly, reset the X position
            position.x = (zi % 2 == 0) ? (min_extent.x - step) : min_extent.x;
            for (int xi = 0; xi < samples_x; ++xi)
            {
                // i tested logic for skipping out points whose values will never be used, but it was actually less efficient!
                sample_values[index] = sampler(position);
#if defined DEBUG_GRID
                sample_positions[index] = position;
#endif
                position.x += resolution;
                ++index;
            }
            position.y += resolution;
        }
        // move along by one sample point
        position.z += step;
    }
}

inline Vector3 MTVT::Builder::clampToBounds(Vector3 v)
{
    return max(min(v, max_extent), min_extent);
}

// each entry defines the set of either 4 or 6 edges which are closest 
// to the edge used to index the array
static constexpr EdgeAddr edge_neighbour_addresses[14][6] =
{
    { PXPYPZ, PXNYPZ, PXPYNZ, PXNYNZ, EDGE_NULL },      // PX
    { NXPYPZ, NXNYPZ, NXPYNZ, NXNYNZ, EDGE_NULL },      // NX
    { PXPYPZ, NXPYPZ, PXPYNZ, NXPYNZ, EDGE_NULL },      // PY
    { PXNYPZ, NXNYPZ, PXNYNZ, NXNYNZ, EDGE_NULL },      // NY
    { PXPYPZ, NXPYPZ, PXNYPZ, NXNYPZ, EDGE_NULL },      // PZ
    { PXPYNZ, NXPYNZ, PXNYNZ, NXNYNZ, EDGE_NULL },      // NZ
    { PX,     PY,     PZ,     NXPYPZ, PXNYPZ, PXPYNZ }, // PXPYPZ
    { NX,     PY,     PZ,     PXPYPZ, NXNYPZ, NXPYNZ }, // NXPYPZ
    { PX,     NY,     PZ,     NXNYPZ, PXPYPZ, PXNYNZ }, // PXNYPZ
    { NX,     NY,     PZ,     PXNYPZ, NXPYPZ, NXNYNZ }, // NXNYPZ
    { PX,     PY,     NZ,     NXPYNZ, PXNYNZ, PXPYPZ }, // PXPYNZ
    { NX,     PY,     NZ,     PXPYNZ, NXNYNZ, NXPYPZ }, // NXPYNZ
    { PX,     NY,     NZ,     NXNYNZ, PXPYNZ, PXNYPZ }, // PXNYNZ
    { NX,     NY,     NZ,     PXNYNZ, NXPYNZ, NXNYPZ }, // NXNYNZ
};

// essentially a bitmask version of the table above!
static constexpr EdgeFlags edge_neighbour_masks[14] =
{  // diag......perp..
    0b0001010101000000,     // PX
    0b0010101010000000,     // NX
    0b0000110011000000,     // PY
    0b0011001100000000,     // NY
    0b0000001111000000,     // PZ
    0b0011110000000000,     // NZ
    0b0000010110010101,     // PXPYPZ
    0b0000101001010110,     // NXPYPZ
    0b0001001001011001,     // PXNYPZ
    0b0010000110011010,     // NXNYPZ
    0b0001100001100101,     // PXPYNZ
    0b0010010010100110,     // NXPYNZ
    0b0001101000101001,     // PXNYNZ
   // diag......perp..
};

#define VERTEX_POSITION(vec, td, van, val, pos) ((vec * (td / (van - val))) + pos)

// this macro simply turns an edge address into the edge address pointing in the 
// opposite direction
#define INVERT_EDGE_INDEX(i) (EdgeAddr)((i < 6) ? (i + 1 - ((i % 2) * 2)) : (19 - i))

inline VertexRef Builder::addVertex(const float* neighbour_values, const EdgeAddr p, const float thresh_diff, const float value, const Vector3& position, vector<Vector3>& verts)
{
    float value_at_neighbour = neighbour_values[p];
    Vector3 vertex_position = VERTEX_POSITION(vector_offsets[p], thresh_diff, value_at_neighbour, value, position);

    verts.push_back(clampToBounds(vertex_position));
    return static_cast<VertexRef>(vertices.size() - 1);
}

inline VertexRef Builder::addMergedVertex(const float* neighbour_values, const float thresh_diff, const float value, const Vector3& position, EdgeFlags usable_edges, vector<Vector3>& verts, EdgeReferences& edges)
{
    Vector3 vertex = { 0, 0, 0 };
    VertexRef ref = static_cast<VertexRef>(vertices.size());
    EdgeFlags mask = 1;
    int merged_count = 0;
    for (EdgeAddr p = 0; p < 14u; ++p, mask <<= 1)
    {
        if (!(usable_edges & mask))
            continue;
        ++merged_count;
        edges.references[p] = ref;
        vertex += VERTEX_POSITION(vector_offsets[p], thresh_diff, neighbour_values[p], value, position);
    }
    vertices.push_back(clampToBounds(vertex / static_cast<float>(merged_count)));

    return ref;
}

inline void Builder::addVerticesIndividually(const float* neighbour_values, const float thresh_diff, const float value, const Vector3& position, EdgeFlags usable_edges, vector<Vector3>& verts, EdgeReferences& edges)
{
    EdgeFlags mask = 1;
    for (EdgeAddr p = 0; p < 14u; ++p, mask <<= 1)
    {
        if (!(usable_edges & mask))
            continue;
        edges.references[p] = addVertex(neighbour_values, p, thresh_diff, value, position, vertices);
    }
}

static inline uint8_t fastBitCount(EdgeFlags val)
{
    static constexpr uint8_t counts[16] =
    { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
    return counts[val & 0xf] + counts[(val >> 4) & 0xf] + counts[(val >> 8) & 0xf] + counts[(val >> 12) & 0xf];
}

static inline EdgeAddr ilog2(const unsigned x) {
    return ((8 * sizeof(unsigned)) - (__lzcnt((unsigned)(x))) - 1);
}

// this table contains pairs of edge masks, where the first item is used
// to check for the presence of opposing edges in an edge-flags value,
// and the second can be used to mask out one side of those flags (the
// bitwise inverse of the second value can be used to mask the opposite side)
static constexpr EdgeFlags opposing_edge_masks[7][2] =
{
    { 0b0000000000000011, 0b0001010101010101 }, // PX - NX
    { 0b0000000000001100, 0b0000110011010101 }, // PY - NY
    { 0b0000000000110000, 0b0000001111010101 }, // PZ - NZ
    { 0b0010000001000000, 0b0000010111010101 }, // PXPYPZ - NXNYNZ
    { 0b0001000010000000, 0b0000101011010110 }, // NXPYPZ - PXNYNZ
    { 0b0000100100000000, 0b0001001101011001 }, // PXNYPZ - NXPYNZ
    { 0b0000011000000000, 0b0001110001100101 }  // PXPYNZ - NXNYPZ
    //            npnpnp      ddccbbaazzyyxx
    //    npnpnpnp            npnpnpnp
    //    nnppnnpp            nnppnnpp
    //    nnnnpppp            nnnnpppp
};

void Builder::vertexPass()
{
    // flagging pass - check all of the edges around each sample point, and set the edge flag bits
    // vertex pass - generate vertices for edges with flags set, and merge them where possible, assigning vertex references to these edges

    // our position in the array, saves recomputing this all the time
    float step = resolution / 2.0f;
    Vector3 position;
    Index index = 0;
    Index connected_indices[14] = { 0 };
    EdgeReferences edges;
    EdgeReferences edges_template; for (int p = 0; p < 14; ++p) edges_template.references[p] = VERTEX_NULL;

    bool is_odd_z = true;
    for (int zi = 0; zi < samples_z; ++zi)
    {
        position.z = (zi * step) + (min_extent.z - step);
        is_odd_z = !is_odd_z;
        bool is_min_z = zi <= 1;
        bool is_max_z = zi >= samples_z - 2;
        for (int yi = 0; yi < samples_y; ++yi)
        {
            position.y = (yi * resolution) + (is_odd_z ? min_extent.y : (min_extent.y - step));
            bool is_max_y = yi >= samples_y - 1;
            bool is_min_y = yi <= 0;
            for (int xi = 0; xi < samples_x; ++xi)
            {
                bool is_max_x = xi >= samples_x - 1;
                // check flags for where this sample is within the sample space
                if (is_odd_z && (is_max_x || is_max_y))
                {
                    // early reject if this is just an extra filler point
                    ++index;
                    continue;
                }
                bool is_min_x = xi <= 0;
                if (!is_odd_z)
                {
                    int num_edges = 0;
                    if (is_min_x) ++num_edges;
                    if (is_max_x) ++num_edges;
                    if (is_min_y) ++num_edges;
                    if (is_max_y) ++num_edges;
                    if (is_min_z) ++num_edges;
                    if (is_max_z) ++num_edges;
                    if (num_edges >= 2)
                    {
                        // early reject if this is an edge point
                        ++index;
                        continue;
                    }
                }

                // populate the list of neighbouring indices
                if (!is_odd_z)
                {
                    for (int t = 0; t < 14; ++t)
                        connected_indices[t] = index + index_offsets_evenz[t];
                }
                else
                {
                    for (int t = 0; t < 14; ++t)
                        connected_indices[t] = index + index_offsets_oddz[t];
                }

                // strike out any neighbour which doesn't exist. we do this
                // on the outer faces of the sample cube as the outermost points
                // do not have neighbours in that face's direction (i.e. these
                // indices would be invalid)
                if (is_min_z)
                {
                    connected_indices[NZ] = INDEX_NULL;
                    if (!is_odd_z)
                    {
                        connected_indices[PX] = INDEX_NULL;
                        connected_indices[NX] = INDEX_NULL;
                        connected_indices[PY] = INDEX_NULL;
                        connected_indices[NY] = INDEX_NULL;
                        connected_indices[PXPYNZ] = INDEX_NULL;
                        connected_indices[NXPYNZ] = INDEX_NULL;
                        connected_indices[PXNYNZ] = INDEX_NULL;
                        connected_indices[NXNYNZ] = INDEX_NULL;
                    }
                }
                if (is_min_y)
                {
                    connected_indices[NY] = INDEX_NULL;
                    
                    if (!is_odd_z)
                    {
                        connected_indices[PX] = INDEX_NULL;
                        connected_indices[NX] = INDEX_NULL;
                        connected_indices[PZ] = INDEX_NULL;
                        connected_indices[NZ] = INDEX_NULL;
                        connected_indices[PXNYPZ] = INDEX_NULL;
                        connected_indices[NXNYPZ] = INDEX_NULL;
                        connected_indices[PXNYNZ] = INDEX_NULL;
                        connected_indices[NXNYNZ] = INDEX_NULL;
                    }
                }
                if (is_min_x)
                {
                    connected_indices[NX] = INDEX_NULL;
                    if (!is_odd_z)
                    {
                        connected_indices[PY] = INDEX_NULL;
                        connected_indices[NY] = INDEX_NULL;
                        connected_indices[PZ] = INDEX_NULL;
                        connected_indices[NZ] = INDEX_NULL;
                        connected_indices[NXPYPZ] = INDEX_NULL;
                        connected_indices[NXNYPZ] = INDEX_NULL;
                        connected_indices[NXPYNZ] = INDEX_NULL;
                        connected_indices[NXNYNZ] = INDEX_NULL;
                    }
                }
                if (is_max_z)
                {
                    connected_indices[PZ] = INDEX_NULL;
                    if (!is_odd_z)
                    {
                        connected_indices[PX] = INDEX_NULL;
                        connected_indices[NX] = INDEX_NULL;
                        connected_indices[PY] = INDEX_NULL;
                        connected_indices[NY] = INDEX_NULL;
                        connected_indices[PXPYPZ] = INDEX_NULL;
                        connected_indices[NXPYPZ] = INDEX_NULL;
                        connected_indices[PXNYPZ] = INDEX_NULL;
                        connected_indices[NXNYPZ] = INDEX_NULL;
                    }
                }
                if (is_max_y)
                {
                    connected_indices[PY] = INDEX_NULL;
                    if (!is_odd_z)
                    {
                        connected_indices[PX] = INDEX_NULL;
                        connected_indices[NX] = INDEX_NULL;
                        connected_indices[PZ] = INDEX_NULL;
                        connected_indices[NZ] = INDEX_NULL;
                        connected_indices[PXPYPZ] = INDEX_NULL;
                        connected_indices[NXPYPZ] = INDEX_NULL;
                        connected_indices[PXPYNZ] = INDEX_NULL;
                        connected_indices[NXPYNZ] = INDEX_NULL;
                    }
                }
                if (is_max_x)
                {
                    connected_indices[PX] = INDEX_NULL;
                    if (!is_odd_z)
                    {
                        connected_indices[PY] = INDEX_NULL;
                        connected_indices[NY] = INDEX_NULL;
                        connected_indices[PZ] = INDEX_NULL;
                        connected_indices[NZ] = INDEX_NULL;
                        connected_indices[PXPYPZ] = INDEX_NULL;
                        connected_indices[PXNYPZ] = INDEX_NULL;
                        connected_indices[PXPYNZ] = INDEX_NULL;
                        connected_indices[PXNYNZ] = INDEX_NULL;
                    }
                }

                // grab useful data about ourself
                EdgeFlags edge_proximity_flags = 0;
                EdgeFlags edge_crossing_flags = 0;
                float value = sample_values[index];
                float thresh_diff = threshold - value;
                float neighbour_values[14];

                // perform edge flagging, by going through and marking a 
                // corresponding bit for each connected edge which
                // intersects the isosurface (i.e. the neighbour value at
                // the other end of the edge is on the other side of the 
                // threshold), and the intersection is closer to us than 
                // the neighbour. we also update a bitfield for whether
                // the neighbour is just different, and store it, hugely
                // speeding up geometry generation later
                float thresh_dist = thresh_diff;
                bool thresh_less = thresh_dist < 0.0f;
                if (thresh_less) thresh_dist = -thresh_dist;
                EdgeFlags mask = 1;
                for (EdgeAddr p = 0; p < 14u; ++p, mask <<= 1)
                {
                    if (connected_indices[p] == INDEX_NULL)
                        continue;

                    float value_at_neighbour = sample_values[connected_indices[p]];
                    float neighbour_dist = threshold - value_at_neighbour;
                    if ((neighbour_dist < 0.0f) == thresh_less)
                        continue;
                    edge_crossing_flags |= mask;

                    if (thresh_dist > (thresh_less ? neighbour_dist : -neighbour_dist))
                        continue;
                    neighbour_values[p] = value_at_neighbour;
                    edge_proximity_flags |= mask;
                }
                sample_crossing_flags[index] = edge_crossing_flags;
                    
                // perform vertex generation & merging
                memcpy(&edges, &edges_template, sizeof(EdgeReferences));
                // skip this entire sample point if there are no intersections at all
                if (edge_proximity_flags == 0)
                {
                    sample_edge_indices[index] = edges;
                    ++index;
                    continue;
                }
                position.x = (xi * resolution) + (is_odd_z ? min_extent.x : (min_extent.x - step));

                // if not in clustering mode, skip the clustering code!
                if (clustering != ClusteringMode::INTEGRATED)
                {
                    mask = 1;
                    for (EdgeAddr p = 0; p < 14u; ++p, mask <<= 1)
                        if (edge_proximity_flags & mask)
                            edges.references[p] = addVertex(neighbour_values, p, thresh_diff, value, position, vertices);
                    sample_edge_indices[index] = edges;
                    ++index;
                    continue;
                }

                // count how many edges are available to be merged
                EdgeFlags usable_edges = edge_proximity_flags;
                const uint8_t num_flagged_edges = fastBitCount(usable_edges);
                // if only one edge is flagged, do the vertex and 
                // skip onward (no need to traverse the array again)
                if (num_flagged_edges == 1)
                {
                    const EdgeAddr one_edge = ilog2(usable_edges);
                    edges.references[one_edge] = addVertex(neighbour_values, one_edge, thresh_diff, value, position, vertices);
                    sample_edge_indices[index] = edges;
                    ++index;
                    continue;
                }
                // if 12 or more edges are flagged, no merging 
                // and we just do them all individually
                if (num_flagged_edges >= 12)
                {
                    addVerticesIndividually(neighbour_values, thresh_diff, value, position, usable_edges, vertices, edges);
                    sample_edge_indices[index] = edges;
                    ++index;
                    continue;
                }

                // GRAPH THEORY TIME
                // build a graph representing which edges may be merged together.
                // each link in the graph represents a pair of edges which are both
                // 1. neighbours, and 2. both usable.
                // the connectivity graph is an adjacency matrix where each bit
                // represents whether there is a connection between the two
                // edges used to index that bit in the array
                EdgeFlags connectivity_graph[14] = { };
                memcpy(connectivity_graph, edge_neighbour_masks, sizeof(EdgeFlags) * 14);
                // mergeable candidates represents how many edges can be merged with
                // each edge (essentially, how many bits are set in each row of the
                // adjacency matrix)
                uint8_t mergeable_candidates[14] = { 0 };
                uint8_t highest_mergeable_count = 0;
                EdgeAddr highest_counted_edge = EDGE_NULL;
                // iterate over the edges and strike out candidate edges which
                // are not both usable
                mask = 1;
                for (EdgeAddr p = 0; p < 14u; ++p, mask <<= 1)
                {
                    if (!(usable_edges & mask))
                    {
                        connectivity_graph[p] = 0;
                        mergeable_candidates[p] = 0;
                        continue;
                    }
                    else
                        connectivity_graph[p] &= usable_edges;
                    mergeable_candidates[p] = fastBitCount(connectivity_graph[p]);
                    if (mergeable_candidates[p] > highest_mergeable_count)
                    {
                        highest_mergeable_count = mergeable_candidates[p];
                        highest_counted_edge = p;
                    }
                }

                // if there are no mergeable edges anywhere, do them all individually and finish
                if (highest_mergeable_count == 0)
                {
                    addVerticesIndividually(neighbour_values, thresh_diff, value, position, usable_edges, vertices, edges);
                    sample_edge_indices[index] = edges;
                    ++index;
                    continue;
                }

                // if there's an edge where the number of mergeable candidates is equal to 
                // the number of total usable edges - 1 (i.e. all are mergeable to this edge)
                // then merge them all together and finish
                if (highest_mergeable_count == num_flagged_edges - 1)
                {
                    addMergedVertex(neighbour_values, thresh_diff, value, position, usable_edges, vertices, edges);
                    sample_edge_indices[index] = edges;
                    ++index;
                    continue;
                }

                // otherwise, separate the data into islands by traversing to connected
                // neighbours and marking them as part of an island, until all edges are
                // marked. then, we check each island for opposing edges using a bitmask;
                // if the island has no opposing edges it can be safely merged, otherwise
                // it must be split into two new groups based on the two opposing edges,
                // before it can be merged

                // separate the remaining data into islands (continuously connected regions)
                int group_ids[14] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
                int current_group_index = 0;
                int total_grouped_size = 0;
                EdgeAddr current_edge = 0;
                vector<EdgeAddr> edge_queue; edge_queue.reserve(14);
                vector<EdgeFlags> groups; edge_queue.reserve(8);
                EdgeFlags current_group = 0;
                // traverse breadth-first to neighbours, marking each as part of the current group
                // when this is complete, if there are unmarked edges, find the first unmarked
                // and repeat traversal.
                while (total_grouped_size < num_flagged_edges)
                {
                    while (group_ids[current_edge] != -1 || !(usable_edges & (1 << current_edge)))
                        ++current_edge;
                    // add this edge to the current group
                    edge_queue.push_back(current_edge);
                    group_ids[current_edge] = current_group_index;
                    int queue_index = 0;
                    // repeat until we run out
                    while (queue_index < edge_queue.size())
                    {
                        mask = 1;
                        // jump to the current edge in the queue
                        current_edge = edge_queue[queue_index];
                        current_group |= (1 << current_edge);
                        for (EdgeAddr next_edge = 0; next_edge < 14u; ++next_edge, mask <<= 1)
                        {
                            // check this edge for connected neighbours, mark each one
                            // and add it to the queue (if it isn't already marked!)
                            if ((connectivity_graph[current_edge] & mask) && group_ids[next_edge] == -1)
                            {
                                edge_queue.push_back(next_edge);
                                group_ids[next_edge] = current_group_index;
                            }
                        }
                        // step to the next element in the queue
                        ++queue_index;
                    }
                    // reset in case we have to find another island
                    total_grouped_size += edge_queue.size();
                    groups.push_back(current_group);
                    edge_queue.clear();
                    current_edge = 0;
                    current_group = 0;
                    ++current_group_index;
                }

                // next, iterate over the islands, checking each for opposing edges.
                // any islands which do not contain opposing edges can be merged,
                // other islands need to be rebuilt as two groups (using bitmasks 
                // to separate the island into halves)
                // FIXME: change the opposing edge checks to be MORE THAN 180 degrees, not 180. i.e., the maximum traversal distance in the group
                for (EdgeFlags group_mask : groups)
                {
                    int mask_index;
                    for (mask_index = 0; mask_index < 7; ++mask_index)
                    {
                        EdgeFlags mask = opposing_edge_masks[mask_index][0];
                        if ((group_mask & mask) == mask)
                        {
                            // we found an opposing edge! kill it!
                            break;
                        }
                    }
                    //if (mask_index >= 7)
                    //{
                        // all good! merge them!
                        addMergedVertex(neighbour_values, thresh_diff, value, position, group_mask, vertices, edges);
                    //}
                    //else
                    //{
                    //    // split the group
                    //    EdgeFlags half_mask = opposing_edge_masks[mask_index][1];
                    //    EdgeFlags group_a = group_mask & half_mask;
                    //    EdgeFlags group_b = group_mask & ~half_mask;
                    //    addMergedVertex(neighbour_values, thresh_diff, value, position, group_a, vertices, edges);
                    //    addMergedVertex(neighbour_values, thresh_diff, value, position, group_b, vertices, edges);
                    //}
                }

                // write back the sample edge indices and continue to the next sample point
                sample_edge_indices[index] = edges;
                ++index;
            }
        }
    }
}

// each entry defines a collection of indices into the list of neighbouring sample points.
// the 24 entries represent the 24 tetrahedra embedded in each cube.
// the cube center is always assumed to be treated as the first sample point
static constexpr EdgeAddr tetrahedra_sample_index_templates[24][3] =
{
    // +x side
    { PX, PXNYPZ, PXNYNZ },
    { PX, PXNYNZ, PXPYNZ },
    { PX, PXPYNZ, PXPYPZ },
    { PX, PXPYPZ, PXNYPZ },
    // -x side
    { NX, NXPYPZ, NXPYNZ },
    { NX, NXPYNZ, NXNYNZ },
    { NX, NXNYNZ, NXNYPZ },
    { NX, NXNYPZ, NXPYPZ },
    // +y side
    { PY, PXPYPZ, PXPYNZ },
    { PY, PXPYNZ, NXPYNZ },
    { PY, NXPYNZ, NXPYPZ },
    { PY, NXPYPZ, PXPYPZ },
    // -y side
    { NY, NXNYPZ, NXNYNZ },
    { NY, NXNYNZ, PXNYNZ },
    { NY, PXNYNZ, PXNYPZ },
    { NY, PXNYPZ, NXNYPZ },
    // +z side
    { PZ, NXPYPZ, NXNYPZ },
    { PZ, NXNYPZ, PXNYPZ },
    { PZ, PXNYPZ, PXPYPZ },
    { PZ, PXPYPZ, NXPYPZ },
    // -z side
    { NZ, NXNYNZ, NXPYNZ },
    { NZ, NXPYNZ, PXPYNZ },
    { NZ, PXPYNZ, PXNYNZ },
    { NZ, PXNYNZ, NXNYNZ },
};

// each pair of entries defines which of the four sample points involved with a tetrahedron
// make up a particular generic edge. order matters! these are generic indices
// into the array of sample point indices relevant to the current tetrahedron.
static constexpr uint8_t tetrahedra_edge_sample_point_indices[12] =
{
    0, 1,   // edge 0 = center -> pyramid   (CP)
    0, 2,   // edge 1 = center -> upper     (CU)
    0, 3,   // edge 2 = center -> lower     (CL)
    1, 2,   // edge 3 = pyramid -> upper    (PU)
    1, 3,   // edge 4 = pyramid -> lower    (PL)
    2, 3    // edge 5 = upper -> lower      (UL)
};

// each entry defines a collection of generic edge indices relative to a sample point.
// the table above relates the sample points involved for each of the generic edges
// described in this list. each element in an entry can be inverted using the macro,
// and applied relative to the other end of the edge (for example, instead of PX of sp0,
// you would have NX of sp1) in order to find the alternate storage location for the relevant
// data.
// hence, entries represent the 6 edges in the tetrahedron and are ordered
//     CP, CU, CL, PU, PL, UL
static constexpr EdgeAddr tetrahedra_edge_address_templates[24][6] =
{
    // +x side
    { PX, PXNYPZ, PXNYNZ, NXNYPZ, NXNYNZ, NZ },
    { PX, PXNYNZ, PXPYNZ, NXNYNZ, NXPYNZ, PY },
    { PX, PXPYNZ, PXPYPZ, NXPYNZ, NXPYPZ, PZ },
    { PX, PXPYPZ, PXNYPZ, NXPYPZ, NXNYPZ, NY },
    // -x side
    { NX, NXPYPZ, NXPYNZ, PXPYPZ, PXPYNZ, NZ },
    { NX, NXPYNZ, NXNYNZ, PXPYNZ, PXNYNZ, NY },
    { NX, NXNYNZ, NXNYPZ, PXNYNZ, PXNYPZ, PZ },
    { NX, NXNYPZ, NXPYPZ, PXNYPZ, PXPYPZ, PY },
    // +y side
    { PY, PXPYPZ, PXPYNZ, PXNYPZ, PXNYNZ, NZ },
    { PY, PXPYNZ, NXPYNZ, PXNYNZ, NXNYNZ, NX },
    { PY, NXPYNZ, NXPYPZ, NXNYNZ, NXNYPZ, PZ },
    { PY, NXPYPZ, PXPYPZ, NXNYPZ, PXNYPZ, PX },
    // -y side
    { NY, NXNYPZ, NXNYNZ, NXPYPZ, NXPYNZ, NZ },
    { NY, NXNYNZ, PXNYNZ, NXPYNZ, PXPYNZ, PX },
    { NY, PXNYNZ, PXNYPZ, PXPYNZ, PXPYPZ, PZ },
    { NY, PXNYPZ, NXNYPZ, PXPYPZ, NXPYPZ, NX },
    // +z side
    { PZ, NXPYPZ, NXNYPZ, NXPYNZ, NXNYNZ, NY },
    { PZ, NXNYPZ, PXNYPZ, NXNYNZ, PXNYNZ, PX },
    { PZ, PXNYPZ, PXPYPZ, PXNYNZ, PXPYNZ, PY },
    { PZ, PXPYPZ, NXPYPZ, PXPYNZ, NXPYNZ, NX },
    // -z side
    { NZ, NXNYNZ, NXPYNZ, NXNYPZ, NXPYPZ, PY },
    { NZ, NXPYNZ, PXPYNZ, NXPYPZ, PXPYPZ, PX },
    { NZ, PXPYNZ, PXNYNZ, PXPYPZ, PXNYPZ, NY },
    { NZ, PXNYNZ, NXNYNZ, PXNYPZ, NXNYPZ, NX }
};

// look up table for the geometry patterns for different tetrahedra configurations
// edge indices range from 0-5, but each one could refer to the inverted-direction version
// from the sample point at the other end of the edge, and this is checked at each step.
// these sequences combine the per-edge vertex references into triangles.
// the last values may be -1 (aka 255) if there is only one triangle
static constexpr uint8_t tetrahedral_edge_address_patterns[16][4] =
{
    { -1, -1, -1, -1 }, // no bits set
    {  0,  1,  2, -1 }, // 0b0001
    {  0,  4,  3, -1 }, // 0b0010
    {  2,  4,  1,  3 }, // 0b0011
    {  3,  5,  1, -1 }, // 0b0100
    {  5,  2,  3,  0 }, // 0b0101
    {  0,  4,  1,  5 }, // 0b0110
    {  5,  2,  4, -1 }, // 0b0111
    {  5,  4,  2, -1 }, // 0b1000
    {  5,  4,  1,  0 }, // 0b1001
    {  0,  2,  3,  5 }, // 0b1010
    {  1,  5,  3, -1 }, // 0b1011
    {  4,  2,  3,  1 }, // 0b1100
    {  0,  3,  4, -1 }, // 0b1101
    {  1,  0,  2, -1 }, // 0b1110
    { -1, -1, -1, -1 }, // all bits set
};

void Builder::geometryPass()
{
    // geometry pass - generate per-tetrahedron geometry from the 
    // edge/sample point info, discard triangles with zero size, 
    // skip sample cubes with no edge crossings.

    Index connected_indices[14] = { 0 };
    for (int zi = 0; zi < cubes_z; ++zi)
    {
        for (int yi = 0; yi < cubes_y; ++yi)
        {
            for (int xi = 0; xi < cubes_x; ++xi)
            {
                // compute central sample point index
                const Index central_sample_index = (2ull * zi * samples_x * samples_y) + (static_cast<size_t>(yi) * samples_x) + (xi)
                                                         + 1 + samples_x + (2ull * samples_x * samples_y);
                // fetch information about which of the neighbours are on
                // the other side of the threshold
                const EdgeFlags central_sample_crossing_flags = sample_crossing_flags[central_sample_index];
                // if the entire cube has no crossings, we can just skip it!
                if (central_sample_crossing_flags == 0)
                    continue; // HUGE SPEEDUP!! 0.03538 -> 0.00412
                // compute all the neighbouring indices in this lattice segment
                for (int e = 0; e < 14; ++e)
                    connected_indices[e] = central_sample_index + index_offsets_evenz[e];

                const bool center_greater_thresh = (sample_values[central_sample_index] > threshold);

                // skip out some tetrahedra depending where we are in the lattice,
                // otherwise we'll be marching lots of tetrahedra twice over
                uint32_t tflags = 0;
                if (xi > 0)
                    tflags |= 0b000000000000000011110000;
                if (yi > 0)
                    tflags |= 0b000000001111000000000000;
                if (zi > 0)
                    tflags |= 0b111100000000000000000000;

                // 24 tetrahedra per cube
                // each tetrahedra has sample point indices generated from its the current cube position (xi,yi,zi)
                for (int t = 0; t < 24; ++t)
                {
                    if (tflags & (1 << t))
                        continue;

                    tetrahedra_evaluated++;
                    
                    // collect the four sample point indices involved with this tetrahedron,
                    // specific to this orientation of tetrahedron (i.e. the first 4 are on
                    // the +x side of the cube, etc)
                    // we make the indices generic, such that all possible tetrahedra are
                    // arranged uniformly to minimise special handling (index 0 is always
                    // the cube center, index 1 is always the SP sticking out in the relevant
                    // direction, index 2 is the clockwise SP when looking at the relevant cube
                    // face, index 3 is the counter-clockwise SP when looking at the relevant
                    // cube face).
                    const Index tetrahedra_sample_indices[4] =
                    {
                        central_sample_index,                                         // C = center
                        connected_indices[(tetrahedra_sample_index_templates[t])[0]], // P = pyramid
                        connected_indices[(tetrahedra_sample_index_templates[t])[1]], // U = upper
                        connected_indices[(tetrahedra_sample_index_templates[t])[2]]  // L = lower
                    };

                    // TODO: should we cache this before the per-tetrahedron loop?
                    const bool sample_neighbours_crossing_flags[3] =
                    {
                        central_sample_crossing_flags & (1 << (tetrahedra_sample_index_templates[t])[0]),
                        central_sample_crossing_flags & (1 << (tetrahedra_sample_index_templates[t])[1]),
                        central_sample_crossing_flags & (1 << (tetrahedra_sample_index_templates[t])[2])
                    };

                    // check which SPs are inside/outside and use that to build a pattern
                    // which identifies this tetrahedral configuration for geometry generation
                    const uint8_t pattern_ident =
                        (center_greater_thresh ? 1 : 0) +
                        ((sample_neighbours_crossing_flags[0] != center_greater_thresh) ? 2 : 0) +
                        ((sample_neighbours_crossing_flags[1] != center_greater_thresh) ? 4 : 0) +
                        ((sample_neighbours_crossing_flags[2] != center_greater_thresh) ? 8 : 0);
                    if (pattern_ident == 0 || pattern_ident == 0b1111)
                        continue;

                    // collect the 12 relevant edge addresses (6 pairs, since one of each pair 
                    // will be filled). this is essentially an expansion of the template for 
                    // edge addresses for this tetrahedron (t).
                    // these will then be used to read data out of the correct edges on each
                    // of the sample points for this tetrahedron
                    const EdgeAddr tetrahedra_edge_addresses[12] =
                    {
                                          tetrahedra_edge_address_templates[t][0],  // relative to C
                        INVERT_EDGE_INDEX(tetrahedra_edge_address_templates[t][0]), // relative to P
                                          tetrahedra_edge_address_templates[t][1],  // relative to C
                        INVERT_EDGE_INDEX(tetrahedra_edge_address_templates[t][1]), // relative to U
                                          tetrahedra_edge_address_templates[t][2],  // relative to C
                        INVERT_EDGE_INDEX(tetrahedra_edge_address_templates[t][2]), // relative to L
                                          tetrahedra_edge_address_templates[t][3],  // relative to P
                        INVERT_EDGE_INDEX(tetrahedra_edge_address_templates[t][3]), // relative to U
                                          tetrahedra_edge_address_templates[t][4],  // relative to P
                        INVERT_EDGE_INDEX(tetrahedra_edge_address_templates[t][4]), // relative to L
                                          tetrahedra_edge_address_templates[t][5],  // relative to U
                        INVERT_EDGE_INDEX(tetrahedra_edge_address_templates[t][5]), // relative to L
                    };

                    // find the edge usage sequence (and thus index sequence) based on the pattern
                    auto pattern = tetrahedral_edge_address_patterns[pattern_ident];
                    // build one or two triangles
                    VertexRef triangle_indices[4] = { VERTEX_NULL, VERTEX_NULL, VERTEX_NULL, VERTEX_NULL };
                    const bool two_triangles = pattern[3] != (uint8_t)-1;
                    const int imax = (two_triangles ? 4 : 3);
                    for (int i = 0; i < imax; ++i)
                    {
                        // this represents the index into the array of edge addresses
                        const uint8_t edge_address_index = pattern[i];
                        // these find the edge addresses for the two alternate 
                        // interpretations of the given edge index, each of which
                        // is relative to one of the sample points on the edge
                        const EdgeAddr edge_address_a = tetrahedra_edge_addresses[edge_address_index * 2];
                        const EdgeAddr edge_address_b = tetrahedra_edge_addresses[(edge_address_index * 2) + 1];
                        // these find the two sample points which are at 
                        // either end of the given edge index
                        const Index sample_point_index_a = tetrahedra_sample_indices[tetrahedra_edge_sample_point_indices[edge_address_index * 2]];
                        const Index sample_point_index_b = tetrahedra_sample_indices[tetrahedra_edge_sample_point_indices[(edge_address_index * 2) + 1]];

                        // assume initial interpretation, but if there is no data on 
                        // that edge, use the alternative interpretation
                        VertexRef vertex_ref = sample_edge_indices[sample_point_index_a].references[edge_address_a];
                        if (vertex_ref == VERTEX_NULL)
                            vertex_ref = sample_edge_indices[sample_point_index_b].references[edge_address_b];

                        triangle_indices[i] = vertex_ref;
                    }

                    // add the generated triangles to the index buffer, checking
                    // for degenerate triangles (i.e. where two or more vertices
                    // are the same)
                    if (triangle_indices[1] == triangle_indices[2])
                    {
                        degenerate_triangles = degenerate_triangles + 2;
                        continue;
                    }

                    if (triangle_indices[0] == triangle_indices[1]
                        || triangle_indices[0] == triangle_indices[2])
                        ++degenerate_triangles;
                    else if (triangle_indices[0] == VERTEX_NULL
                        || triangle_indices[1] == VERTEX_NULL
                        || triangle_indices[2] == VERTEX_NULL)
                        ++invalid_triangles;
                    else
                    {
                        indices.push_back(triangle_indices[0]);
                        indices.push_back(triangle_indices[1]);
                        indices.push_back(triangle_indices[2]);
                    }

                    if (two_triangles)
                    {
                        if (triangle_indices[3] == triangle_indices[1]
                            || triangle_indices[3] == triangle_indices[2])
                            ++degenerate_triangles;
                        else if (triangle_indices[3] == VERTEX_NULL
                            || triangle_indices[1] == VERTEX_NULL
                            || triangle_indices[2] == VERTEX_NULL)
                            ++invalid_triangles;
                        else
                        {
                            indices.push_back(triangle_indices[3]);
                            indices.push_back(triangle_indices[2]);
                            indices.push_back(triangle_indices[1]);
                        }
                    }
                }
            }
        }
    }
}

void Builder::computeVertexNormals()
{
    if (indices.empty())
        return;

    normals.resize(vertices.size());

    for (size_t i = 0; i < indices.size() - 2; i += 3)
    {
        const VertexRef i0 = indices[i];
        const VertexRef i1 = indices[i + 1];
        const VertexRef i2 = indices[i + 2];

        const Vector3 v0 = vertices[i0];
        const Vector3 v1 = vertices[i1];
        const Vector3 v2 = vertices[i2];

        Vector3 e01 = v1 - v0;
        Vector3 e02 = v2 - v0;
        Vector3 e12 = v2 - v1;
        Vector3 normal = e01 % e02;

        // this weighting system produces really fucked normals when the triangles are tiny.
        // and it also costs a really large amount of compute time
        /*float w0 = angle(e01, e02);
        float w1 = angle(e12, -e01);
        float w2 = angle(-e02, -e12);*/

        normals[i0] += normal;// *w0;
        normals[i1] += normal;// *w1;
        normals[i2] += normal;// *w2;
    }

    for (Vector3& normal : normals)
        normal = norm(normal);
}

// TODO: different lattice structures
// TODO: different merging techniques
