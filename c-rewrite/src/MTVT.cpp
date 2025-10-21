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

    {
        // generate a set of index offsets for surrounding sample points,
        // used in the flagging pass
        // the bit-flagging is as follows:
        int s_y = samples_x;
        int s_hz = samples_y * s_y;
        int s_z = s_hz * 2;

        index_offsets_evenz[0]  =  1;               //  0 - 0b00000000 00000001 - +X
        index_offsets_evenz[1]  = -1;               //  1 - 0b00000000 00000010 - -X
        index_offsets_evenz[2]  =  s_y;             //  2 - 0b00000000 00000100 - +Y
        index_offsets_evenz[3]  = -s_y;             //  3 - 0b00000000 00001000 - -Y
        index_offsets_evenz[4]  =  s_z;             //  4 - 0b00000000 00010000 - +Z
        index_offsets_evenz[5]  = -s_z;             //  5 - 0b00000000 00100000 - -Z

        index_offsets_evenz[6]  =  s_hz;            //  6 - 0b00000000 01000000 - +X+Y+Z
        index_offsets_evenz[7]  =  s_hz - 1;        //  7 - 0b00000000 10000000 - -X+Y+Z
        index_offsets_evenz[8]  =  s_hz - s_y;      //  8 - 0b00000001 00000000 - +X-Y+Z
        index_offsets_evenz[9]  =  s_hz - s_y - 1;  //  9 - 0b00000010 00000000 - -X-Y+Z
        index_offsets_evenz[10] = -s_hz;            // 10 - 0b00000100 00000000 - +X+Y-Z
        index_offsets_evenz[11] = -s_hz - 1;        // 11 - 0b00001000 00000000 - -X+Y-Z
        index_offsets_evenz[12] = -s_hz - s_y;      // 12 - 0b00010000 00000000 - +X-Y-Z
        index_offsets_evenz[13] = -s_hz - s_y - 1;  // 13 - 0b00100000 00000000 - -X-Y-Z

        index_offsets_oddz[0]   = index_offsets_evenz[0];
        index_offsets_oddz[1]   = index_offsets_evenz[1];
        index_offsets_oddz[2]   = index_offsets_evenz[2];
        index_offsets_oddz[3]   = index_offsets_evenz[3];
        index_offsets_oddz[4]   = index_offsets_evenz[4];
        index_offsets_oddz[5]   = index_offsets_evenz[5];

        index_offsets_oddz[6]   = s_hz + s_y + 1;
        index_offsets_oddz[7]   = s_hz + s_y;
        index_offsets_oddz[8]   = s_hz + 1;
        index_offsets_oddz[9]   = s_hz;
        index_offsets_oddz[10]  = -s_hz + s_y + 1;
        index_offsets_oddz[11]  = -s_hz + s_y;
        index_offsets_oddz[12]  = -s_hz + 1;
        index_offsets_oddz[13]  = -s_hz;
    }
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

    auto flagging_start = chrono::high_resolution_clock::now();
    flaggingPass();
    float flagging = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - flagging_start)).count();

    auto vertex_start = chrono::high_resolution_clock::now();
    vertexPass();
    float vertex = ((chrono::duration<float>)(chrono::high_resolution_clock::now() - vertex_start)).count();

    stats.cubes_x = cubes_x;
    stats.cubes_y = cubes_y;
    stats.cubes_z = cubes_z;
    stats.allocation_time += allocation;
    stats.sampling_time += sampling;
    stats.flagging_time += flagging;
    stats.vertex_time += vertex;
    stats.sample_points = grid_data_length;
    stats.vertices = vertices.size();

    destroyBuffers();

    return MTVTMesh();
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

void MTVTBuilder::flaggingPass()
{
    // flagging pass - check all of the edges around each sample point, and set the edge flag bits

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

                uint16_t bits = 0;
                float value = sample_values[index];
                float thresh_dist = threshold - value;
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
                    // FIXME: should we compute the edge position and store it? it might save some math and some memory lookups
                    if (thresh_dist < neighbour_dist)
                        bits |= (1 << p);
                }
                sample_proximity_flags[index] = bits;
                ++index;
            }
        }
    }
}

void MTVTBuilder::vertexPass()
{
    // vertex pass - generate vertices for edges with flags set, and merge them where possible, assigning vertex references to these edges

    // our position in the array, saves recomputing this all the time
    size_t index = 0;
    size_t connected_indices[14] = { 0 };

    // FIXME: we're doing a bunch of checking and fetching and iterating twice over! we should merge this pass with the with the flagging pass
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

                // TODO: implement merging!
                // FIXME: oh shit! we need positions here!
                uint16_t bits = sample_proximity_flags[index];
                EdgeReferences edges;
                float value = sample_values[index];
                float thresh_diff = threshold - value;
                Vector3 position = sample_positions[index];
                for (int p = 0; p < 14; ++p)
                {
                    if (connected_indices[p] == (size_t)-1)
                        continue;
                    if (!(bits & (1 << p)))
                        continue;
                    float value_at_neighbour = sample_values[connected_indices[p]];
                    Vector3 position_at_neighbour = sample_positions[p];
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

// TODO: different lattice structures
// TODO: different merging techniques
// TODO: parallelise
// TODO: time/memory tracking
// TODO: vertex/index count