#include "MTVT.h"

#include <chrono>

using namespace std;

MTVTBuilder::MTVTBuilder()
{
	configure({ -1, -1, -1 }, { 1, 1, 1 }, 0.1f, [](Vector3 v) -> float { return mag(v); }, 1.0f);
	configureModes(LatticeType::BODY_CENTERED_DIAMOND, ClusteringMode::NONE);
}

void MTVTBuilder::configure(Vector3 minimum_extent, Vector3 maximum_extent, float cube_size, float(*sample_func)(Vector3), float threshold_value)
{
    sampler = sample_func;
    threshold = threshold_value;
    resolution = abs(cube_size);
    min_extent = min(minimum_extent, maximum_extent);
    max_extent = max(minimum_extent, maximum_extent);
    size = max_extent - min_extent;
    cubes_x = (int)ceilf(size.x / resolution);
    cubes_y = (int)ceilf(size.y / resolution);
    cubes_z = (int)ceilf(size.z / resolution);
    // sample points will contain space for the entire lattice
    // this means we need space for cubes_s + 1 + cubes_s + 2 points for the BCDL
    // and every other layer in each direction is one sample shorter (and we just leave the last one blank)
    samples_x = cubes_x + 2;
    samples_y = cubes_y + 2;
    samples_z = (cubes_z * 2) + 3;
    grid_data_length = static_cast<size_t>(samples_x) * static_cast<size_t>(samples_y) * static_cast<size_t>(samples_z);

    populateIndexOffsets();
}

void MTVTBuilder::configureModes(LatticeType lattice_type, ClusteringMode clustering_mode)
{
    structure = lattice_type;
    clustering = clustering_mode;
}

MTVTMesh MTVTBuilder::generate(MTVTDebugStats& stats)
{
    if (sampler == nullptr)
        return MTVTMesh();

    auto allocation_start = chrono::high_resolution_clock::now();
    prepareBuffers();
    vertices.clear(); // TODO: test size reservation for speed
    indices.clear();
    float allocation = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - allocation_start)).count();

    auto sampling_start = chrono::high_resolution_clock::now();
    samplingPass();
    float sampling = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - sampling_start)).count();

    auto vertex_start = chrono::high_resolution_clock::now();
    vertexPass();
    float vertex = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - vertex_start)).count();

    auto geometry_start = chrono::high_resolution_clock::now();
    geometryPass();
    float geometry = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - geometry_start)).count();

    stats.cubes_x = cubes_x;
    stats.cubes_y = cubes_y;
    stats.cubes_z = cubes_z;
    stats.allocation_time += allocation;
    stats.sampling_time += sampling;
    stats.vertex_time += vertex;
    stats.geometry_time += geometry;
    stats.sample_points_allocated = grid_data_length;
    stats.edges_allocated = grid_data_length * 14;
    stats.min_sample_points = (2 * stats.cubes_x * stats.cubes_y * stats.cubes_z)
                                + (3 * stats.cubes_x * stats.cubes_y) + (3 * stats.cubes_y * stats.cubes_z) + (3 * stats.cubes_x * stats.cubes_z)
                                + stats.cubes_x + stats.cubes_y + stats.cubes_z
                                + 1;
    stats.min_edges         = (14 * stats.cubes_x * stats.cubes_y * stats.cubes_z)
                                + (11 * stats.cubes_x * stats.cubes_y) + (11 * stats.cubes_y * stats.cubes_z) + (11 * stats.cubes_x * stats.cubes_z)
                                + stats.cubes_x + stats.cubes_y + stats.cubes_z;
    stats.vertices = vertices.size();
    stats.indices = indices.size();

    destroyBuffers();

    return MTVTMesh{ vertices, {}, indices };
}

void MTVTBuilder::prepareBuffers()
{
    // TODO: experiment with un-caching this (recomputing position each step)
    sample_positions = new Vector3[grid_data_length];

    sample_values = new float[grid_data_length];

    sample_proximity_flags = new uint16_t[grid_data_length];

    sample_edge_indices = new EdgeReferences[grid_data_length];
}

void MTVTBuilder::destroyBuffers()
{
    delete[] sample_positions; sample_positions = nullptr;

    delete[] sample_values; sample_values = nullptr;

    delete[] sample_proximity_flags; sample_proximity_flags = nullptr;

    delete[] sample_edge_indices; sample_edge_indices = nullptr;
}

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

void MTVTBuilder::populateIndexOffsets()
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
}

void MTVTBuilder::samplingPass()
{
    // sampling pass - compute the values at all of the sample points
    float step = resolution / 2.0f;

    // our position in the array, saves recomputing this all the time
    size_t index = 0;
    // current sample point position
    Vector3 position = Vector3{ 0, 0, min_extent.z - step };
    for (int zi = 0; zi < samples_z; ++zi)
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
                sample_positions[index] = position;
                position.x += resolution;
                index++;
            }
            position.y += resolution;
        }
        // move along by one sample point
        position.z += step;
    }
}

void MTVTBuilder::vertexPass()
{
    // flagging pass - check all of the edges around each sample point, and set the edge flag bits
    // vertex pass - generate vertices for edges with flags set, and merge them where possible, assigning vertex references to these edges

    // our position in the array, saves recomputing this all the time
    size_t index = 0;
    size_t connected_indices[14] = { 0 };

    bool is_odd_z = true;
    for (int zi = 0; zi < samples_z; ++zi)
    {
        is_odd_z = !is_odd_z;
        bool is_min_z = zi <= 1;
        bool is_max_z = zi >= samples_z - 2;
        for (int yi = 0; yi < samples_y; ++yi)
        {
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

                // strike out any neighbour which doesn't exist
                if (is_min_z)
                {
                    connected_indices[0] = -1;
                    connected_indices[1] = -1;
                    connected_indices[2] = -1;
                    connected_indices[3] = -1;
                    connected_indices[5] = -1;
                    connected_indices[10] = -1;
                    connected_indices[11] = -1;
                    connected_indices[12] = -1;
                    connected_indices[13] = -1;
                }
                if (is_min_y)
                {
                    connected_indices[0] = -1;
                    connected_indices[1] = -1;
                    connected_indices[3] = -1;
                    connected_indices[4] = -1;
                    connected_indices[5] = -1;
                    connected_indices[8] = -1;
                    connected_indices[9] = -1;
                    connected_indices[12] = -1;
                    connected_indices[13] = -1;
                }
                if (is_min_x)
                {
                    connected_indices[1] = -1;
                    connected_indices[2] = -1;
                    connected_indices[3] = -1;
                    connected_indices[4] = -1;
                    connected_indices[5] = -1;
                    connected_indices[7] = -1;
                    connected_indices[9] = -1;
                    connected_indices[11] = -1;
                    connected_indices[13] = -1;
                }
                if (is_max_z)
                {
                    connected_indices[0] = -1;
                    connected_indices[1] = -1;
                    connected_indices[2] = -1;
                    connected_indices[3] = -1;
                    connected_indices[4] = -1;
                    connected_indices[6] = -1;
                    connected_indices[7] = -1;
                    connected_indices[8] = -1;
                    connected_indices[9] = -1;
                }
                if (is_max_y)
                {
                    connected_indices[0] = -1;
                    connected_indices[1] = -1;
                    connected_indices[2] = -1;
                    connected_indices[4] = -1;
                    connected_indices[5] = -1;
                    connected_indices[6] = -1;
                    connected_indices[7] = -1;
                    connected_indices[10] = -1;
                    connected_indices[11] = -1;
                }
                if (is_max_x)
                {
                    connected_indices[0] = -1;
                    connected_indices[2] = -1;
                    connected_indices[3] = -1;
                    connected_indices[4] = -1;
                    connected_indices[5] = -1;
                    connected_indices[6] = -1;
                    connected_indices[8] = -1;
                    connected_indices[10] = -1;
                    connected_indices[12] = -1;
                }

                // grab useful data about ourself
                uint16_t bits = 0;
                float value = sample_values[index];
                float thresh_diff = threshold - value;
                float neighbour_values[14];

                // perform edge flagging
                float thresh_dist = thresh_diff;
                bool thresh_less = thresh_dist < 0.0f;
                if (thresh_less) thresh_dist = -thresh_dist;
                for (int p = 0; p < 14; ++p)
                {
                    if (connected_indices[p] == (size_t)-1)
                        continue;
                    float value_at_neighbour = sample_values[connected_indices[p]];
                    float neighbour_dist = threshold - value_at_neighbour;
                    if (neighbour_dist < 0.0f == thresh_less)
                        continue;
                    if (!thresh_less) neighbour_dist = -neighbour_dist;
                    if (thresh_dist > neighbour_dist)
                        continue;

                    neighbour_values[p] = value_at_neighbour;
                    bits |= (1 << p);
                }
                sample_proximity_flags[index] = bits; // TODO: do we actually need to store this at all?

                // perform vertex generation & merging
                // TODO: actually implement merging!
                EdgeReferences edges; for (int p = 0; p < 14; ++p) edges.references[p] = -1;
                if (bits == 0)
                {
                    // skip this entire sample point if there are no intersections at all
                    sample_edge_indices[index] = edges;
                    ++index;
                    continue;
                }
                Vector3 position = sample_positions[index];
                for (int p = 0; p < 14; ++p)
                {
                    if (!(bits & (1 << p)))
                        continue;
                    float value_at_neighbour = neighbour_values[p];
                    Vector3 position_at_neighbour = sample_positions[connected_indices[p]];
                    // TODO: can this be accelerated by rearrangement?
                    Vector3 vertex_position = ((position_at_neighbour - position) * (thresh_diff / (value_at_neighbour - value))) + position;
                    vertices.push_back(vertex_position);
                    edges.references[p] = vertices.size() - 1;
                }
                sample_edge_indices[index] = edges;
                ++index;
            }
        }
    }
}

static uint8_t tetrahedra_sample_indices_templates[24][3] =
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

// each entry is ordered accordingly:
// c = center, p = pyramid, u = upper, l = lower
// the indices for inverted direction can be computed from the next table
//    cp, cu, cl, pu, pl, ul
static uint8_t tetrahedra_edge_indices_templates[24][6] =
{
    // +x side
    { PX, PXNYPZ, PXNYNZ, NXNYPZ, NXNYNZ, NZ },
    { PX, PXNYNZ, PXPYNZ, NXNYNZ, PXNYNZ, PY },
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

// TODO: can we replace this with arithmetic to be faster?
static uint8_t inverted_edge_indices[14] =
{
    1,
    0,
    3,
    2,
    5,
    4,

    13,
    12,
    11,
    10,
    9,
    8,
    7,
    6,
};

// look up table for the edge patterns for different tetrahedra configurations
// edge indices range from 0-5, but each one could refer to the inverted-direction version,
// and this has to be checked at each step
// the last values may be -1 (aka 255) if there is only one triangle
// TODO: can we cut this in half?
// TODO: can we reduce the size of double-triangle entries to only 4 indices?
static uint8_t tetrahedral_edge_index_index_patterns[16][6] =
{
    { -1, -1, -1, -1, -1, -1 },                 // no bits set
    {  0,  1,  2, -1, -1, -1 },                 // 0b0001
    {  0,  4,  3, -1, -1, -1 },                 // 0b0010
    {  2,  4,  1,  3,  1,  4 },                 // 0b0011
    {  3,  5,  1, -1, -1, -1 },                 // 0b0100
    {  5,  2,  3,  0,  3,  2 },                 // 0b0101
    {  0,  4,  1,  5,  1,  4 },                 // 0b0110
    {  5,  2,  4, -1, -1, -1 },                 // 0b0111
    {  5,  4,  2, -1, -1, -1 },                 // 0b1000
    {  5,  4,  1,  0,  1,  4 },                 // 0b1001
    {  0,  2,  3,  5,  3,  2 },                 // 0b1010
    {  1,  5,  3, -1, -1, -1 },                 // 0b1011
    {  4,  2,  3,  1,  3,  2 },                 // 0b1100
    {  0,  3,  4, -1, -1, -1 },                 // 0b1101
    {  1,  0,  2, -1, -1, -1 },                 // 0b1110
    { -1, -1, -1, -1, -1, -1 },                 // all bits set
};

// this mirrors the CP, CU, CL, PU, PL, UL from above
static uint8_t sample_point_edge_indices[12] =
{
    0, 1,
    0, 2,
    0, 3,
    1, 2,
    1, 3,
    2, 3
};

#define INVERT_EDGE_INDEX(i) inverted_edge_indices[i]

void MTVTBuilder::geometryPass()
{
    size_t connected_indices[14] = { 0 };
    for (int zi = 0; zi < cubes_z; ++zi)
    {
        for (int yi = 0; yi < cubes_y; ++yi)
        {
            for (int xi = 0; xi < cubes_x; ++xi)
            {
                // 24 tetrahedra per cube
                // compute central sample point index
                size_t central_sample_point_index = (2ull * zi * samples_x * samples_y) + (static_cast<size_t>(yi) * samples_x) + (xi)
                                                    + 1 + samples_x + (2ull * samples_x * samples_y);
                
                // use that to compute the sample point indices in this lattice segment
                for (int e = 0; e < 14; ++e)
                    connected_indices[e] = central_sample_point_index + index_offsets_evenz[e];

                // each tetrahedra has sample point indices generated from its the current cube position (xi,yi,zi)
                // TODO: skip out some tetrahedra depending where we are in the lattice
                for (int t = 0; t < 24; ++t)
                {
                    // collect the four sample point indices involved with this tetrahedron
                    size_t tetrahedra_sample_indices[4] =
                    {
                        central_sample_point_index,                                     // c = center
                        connected_indices[(tetrahedra_sample_indices_templates[t])[0]], // p = pyramid
                        connected_indices[(tetrahedra_sample_indices_templates[t])[1]], // u = upper
                        connected_indices[(tetrahedra_sample_indices_templates[t])[2]]  // l = lower
                    };
                    // check which SPs are inside/outside and use that to index into tables
                    // with information about which edges of which sample points to use
                    uint8_t pattern_ident =
                        ((sample_values[tetrahedra_sample_indices[0]] > threshold) ? 1 : 0) +
                        ((sample_values[tetrahedra_sample_indices[1]] > threshold) ? 2 : 0) +
                        ((sample_values[tetrahedra_sample_indices[2]] > threshold) ? 4 : 0) +
                        ((sample_values[tetrahedra_sample_indices[3]] > threshold) ? 8 : 0);
                    if (pattern_ident == 0 || pattern_ident == 0b1111)
                        continue;
                    
                    // collect the 12 relevant edge indices (6 pairs, since one of each pair will be filled)
                    // TODO: skip copying this out to an array, we don't need to
                    uint8_t tetrahedra_edge_indices[12] =
                    {
                                          tetrahedra_edge_indices_templates[t][0],  // relative to c
                        INVERT_EDGE_INDEX(tetrahedra_edge_indices_templates[t][0]), // relative to p
                                          tetrahedra_edge_indices_templates[t][1],  // relative to c
                        INVERT_EDGE_INDEX(tetrahedra_edge_indices_templates[t][1]), // relative to u
                                          tetrahedra_edge_indices_templates[t][2],  // relative to c
                        INVERT_EDGE_INDEX(tetrahedra_edge_indices_templates[t][2]), // relative to l
                                          tetrahedra_edge_indices_templates[t][3],  // relative to p
                        INVERT_EDGE_INDEX(tetrahedra_edge_indices_templates[t][3]), // relative to u
                                          tetrahedra_edge_indices_templates[t][4],  // relative to p
                        INVERT_EDGE_INDEX(tetrahedra_edge_indices_templates[t][4]), // relative to l
                                          tetrahedra_edge_indices_templates[t][5],  // relative to u
                        INVERT_EDGE_INDEX(tetrahedra_edge_indices_templates[t][5]), // relative to l
                    };


                    // find the edge usage sequence (and thus index sequence) based on the pattern
                    auto pattern = tetrahedral_edge_index_index_patterns[pattern_ident];
                    // build one or two triangles
                    uint16_t triangle_indices[6] = { -1, -1, -1, -1, -1, -1 };
                    bool two_triangles = true;
                    for (int i = 0; i < 6; i++)
                    {
                        // this represents the index into the array of edge addresses
                        uint8_t edge_index_index = pattern[i];
                        if (edge_index_index == (uint8_t)-1)
                        {
                            two_triangles = false;
                            break;
                        }
                        // these find the edge addresses for the two alternate interpretations of the given edge index
                        uint8_t edge_address_a = tetrahedra_edge_indices[edge_index_index * 2];
                        uint8_t edge_address_b = tetrahedra_edge_indices[(edge_index_index * 2) + 1];
                        // these find the two sample point indices which are at either end of the given edge index
                        size_t sample_point_index_a = tetrahedra_sample_indices[sample_point_edge_indices[edge_index_index * 2]];
                        size_t sample_point_index_b = tetrahedra_sample_indices[sample_point_edge_indices[(edge_index_index * 2) + 1]];

                        // assume initial interpretation, if there is no data on that edge, use the alternative interpretation
                        uint16_t vertex_ref = sample_edge_indices[sample_point_index_a].references[edge_address_a];
                        if (vertex_ref == (uint16_t)-1)
                            vertex_ref = sample_edge_indices[sample_point_index_b].references[edge_address_b];

                        triangle_indices[i] = vertex_ref;
                    }

                    // TODO: check if either one is degenerate

                    indices.push_back(triangle_indices[0]);
                    indices.push_back(triangle_indices[1]);
                    indices.push_back(triangle_indices[2]);
                    if (two_triangles)
                    {
                        indices.push_back(triangle_indices[3]);
                        indices.push_back(triangle_indices[4]);
                        indices.push_back(triangle_indices[5]);
                    }
                }
            }
        }
    }
}

// TODO: different lattice structures
// TODO: different merging techniques
// TODO: parallelise
