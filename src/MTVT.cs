using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

public struct MTVTMesh
{
    public List<Vector3> vertices;
    public List<Vector3> normals;
    public List<uint> indices;
}

public struct MTVTDebugStats
{
    public int cubes_x;
    public int cubes_y;
    public int cubes_z;
    public double allocation_time;
    public double sampling_time;
    public double flagging_time;
    public double vertex_time;
    public double geometry_time;
    public long sample_points;
    public long edges;
    public long tetrahedra;
    public long vertices;
    public long indices;
}

public class MTVTBuilder
{
    private Func<Vector3, float>? sampler;
    private float threshold;
    private Vector3 min_extent;
    private Vector3 max_extent;
    private Vector3 size;
    private int cubes_x;
    private int cubes_y;
    private int cubes_z;
    private int samples_x;
    private int samples_y;
    private int samples_z;
    private float resolution;
    private long grid_data_length;

    private int[] index_offsets_evenz = [];
    private int[] index_offsets_oddz = [];

    private float[] sample_values = [];
    private Vector3[] sample_positions = [];
    private Int16[] sample_proximity_flags = [];

    public MTVTBuilder()
    {
        configure(Vector3.One * -1.0f, Vector3.One, 0.1f, (Vector3 v) => { return v.Length(); }, 1.0f);
        configureModes(true, true, true, 1);
    }

    public void configure(Vector3 minimum_extent, Vector3 maximum_extent, float cube_size, Func<Vector3, float> sample_func, float threshold_value)
    {
        sampler = sample_func;
        threshold = threshold_value;
        resolution = Math.Abs(cube_size);
        min_extent = Vector3.Min(minimum_extent, maximum_extent);
        max_extent = Vector3.Max(minimum_extent, maximum_extent);
        size = max_extent - min_extent;
        cubes_x = (int)Math.Ceiling(size.X / resolution);
        cubes_y = (int)Math.Ceiling(size.Y / resolution);
        cubes_z = (int)Math.Ceiling(size.Z / resolution);
        samples_x = cubes_x + 2;
        samples_y = cubes_y + 2;
        samples_z = (cubes_z * 2) + 3;
        grid_data_length = samples_x * samples_y * samples_z;

        {
            // generate a set of index offsets for surrounding sample points,
            // used in the flagging pass
            // the bit-flagging is as follows:
            int s_y = samples_x;
            int s_hz = samples_y * s_y;
            int s_z = s_hz * 2;
            index_offsets_evenz =
            [
                 1,               //  0 - 0b00000000 00000001 - +X
                -1,               //  1 - 0b00000000 00000010 - -X
                 s_y,             //  2 - 0b00000000 00000100 - +Y
                -s_y,             //  3 - 0b00000000 00001000 - -Y
                 s_z,             //  4 - 0b00000000 00010000 - +Z
                -s_z,             //  5 - 0b00000000 00100000 - -Z

                 s_hz,            //  6 - 0b00000000 01000000 - +X+Y+Z
                 s_hz - 1,        //  7 - 0b00000000 10000000 - -X+Y+Z
                 s_hz - s_y,      //  8 - 0b00000001 00000000 - +X-Y+Z
                 s_hz - s_y - 1,  //  9 - 0b00000010 00000000 - -X-Y+Z
                -s_hz,            // 10 - 0b00000100 00000000 - +X+Y-Z
                -s_hz - 1,        // 11 - 0b00001000 00000000 - -X+Y-Z
                -s_hz - s_y,      // 12 - 0b00010000 00000000 - +X-Y-Z
                -s_hz - s_y - 1,  // 13 - 0b00100000 00000000 - -X-Y-Z
            ];
            index_offsets_oddz =
            [
                index_offsets_evenz[0],
                index_offsets_evenz[1],
                index_offsets_evenz[2],
                index_offsets_evenz[3],
                index_offsets_evenz[4],
                index_offsets_evenz[5],

                 s_hz + s_y + 1,
                 s_hz + s_y,
                 s_hz + 1,
                 s_hz,
                -s_hz + s_y + 1,
                -s_hz + s_y,
                -s_hz + 1,
                -s_hz,
            ];
        }

        {
            sample_values = new float[grid_data_length];
            sample_positions = new Vector3[grid_data_length];
            sample_proximity_flags = new Int16[grid_data_length];
        }
    }

    public void configureModes(bool cache_positions, bool cache_values, bool use_diamond_lattice, int merging_mode)
    {
        // TODO: configurable mode options!
    }

    private void samplingPass()
    {
        // sampling pass - compute the values at all of the sample points
        float step = resolution / 2.0f;

        // TODO: OPTIMISE: we can skip some points out here whose values will never be used (final elements on odd rows)

        // our position in the array, saves recomputing this all the time
        long index = 0;
        // current sample point position
        Vector3 position = new Vector3(0, 0, min_extent.Z - step);
        for (int zi = 0; zi < samples_z; zi++)
        {
            // reset the Y position according to whether this is a key row (zi % 2 = 1)
            // or an off row (zi % 2 = 0). this creates the diamond pattern
            position.Y = (zi % 2 == 0) ? (min_extent.Y - step) : min_extent.Y;
            for (int yi = 0; yi < samples_y; yi++)
            {
                // similarly, reset the X position
                position.X = (zi % 2 == 0) ? (min_extent.X - step) : min_extent.X;
                for (int xi = 0; xi < samples_x; xi++)
                {
                    sample_values[index] = sampler(position);
                    sample_positions[index] = position;
                    position.X += resolution;
                    index++;
                }
                position.Y += resolution;
            }
            // move along by one sample point
            position.Z += step;
        }
    }

    private void flaggingPass()
    {
        // flagging pass - check all of the edges around each sample point, and set the edge flag bits

        // our position in the array, saves recomputing this all the time
        int index = 0;
        int[] connected_indices = new int[14];
        
        for (int zi = 0; zi < samples_z; zi++)
        {
            for (int yi = 0; yi < samples_y; yi++)
            {
                for (int xi = 0; xi < samples_x; xi++)
                {
                    // FIXME: looking at this, it may be possible to reduce the memory footprint of the cube! WILL require a bunch of rewrites though (see blender)

                    // make some flags for where this sample is within the sample space
                    bool is_odd_z = zi % 2 == 1;
                    bool is_max_x = xi >= samples_x - 1;
                    bool is_max_y = yi >= samples_y - 1;
                    if (is_odd_z && (is_max_x || is_max_y))
                    {
                        // early reject if this is just an extra filler point
                        index++;
                        continue;
                    }
                    bool is_min_x = xi <= 0;
                    bool is_min_y = yi <= 0;
                    bool is_min_z = zi <= 1;
                    bool is_max_z = zi >= samples_z - 2;
                    if (!is_odd_z)
                    {
                        int num_edges = 0;
                        if (is_min_x) num_edges++;
                        if (is_max_x) num_edges++;
                        if (is_min_y) num_edges++;
                        if (is_max_y) num_edges++;
                        if (is_min_z) num_edges++;
                        if (is_max_z) num_edges++;
                        if (num_edges >= 2)
                        {
                            // early reject if this is an edge point
                            index++;
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

                    int bits = 0;
                    float value = sample_values[index];
                    float thresh_dist = threshold - value;
                    bool thresh_less = thresh_dist < 0.0f;
                    if (thresh_less) thresh_dist = -thresh_dist;
                    for (int p = 0; p < connected_indices.Length; ++p)
                    {
                        if (connected_indices[p] < 0)
                            continue;
                        float value_at_neighbour = sample_values[connected_indices[p]];
                        float neighbour_dist = threshold - value_at_neighbour;
                        if (neighbour_dist < 0.0f == thresh_less)
                            continue;
                        if (!thresh_less) neighbour_dist = -neighbour_dist;
                        // FIXME: should we compute the edge position and store it? it might save some math and some memory lookups
                        bits |= ((thresh_dist < neighbour_dist) ? (1 << p) : 0);
                    }
                    sample_proximity_flags[index] = (Int16)bits;
                    index++;
                }
            }
        }
    }

    private void vertexPass(/*out uint[] edge_indices, out List<Vector3> vertices, in float[] sample_points, in float[] sample_prox_flags*/)
    {
        // TODO: vertex pass - generate vertices for edges with flags set, and merge them where possible, assigning vertex references to these edges
    }

    private void geometryPass(/*out List<uint> index_buffer, in uint[] edge_indices, in float[] sample_points, Vector3 grid_size*/)
    {
        // TODO: geometry pass - generate per-tetrahedron geometry from the edge/sample point info, discard triangles which zero size
    }

    public MTVTMesh generate(ref MTVTDebugStats stats)
    {
        if (sampler == null)
            return new MTVTMesh();

        // sample points will contain space for the entire lattice
        // this means we need space for cubes_s + 1 + cubes_s + 2 points for the BCDL
        // and every other layer in each direction is one sample shorter (and we just leave the last one blank)
        Stopwatch allocation = Stopwatch.StartNew();
        
        allocation.Stop();

        Stopwatch sampling = Stopwatch.StartNew();
        samplingPass();
        sampling.Stop();

        Stopwatch flagging = Stopwatch.StartNew();
        flaggingPass();
        flagging.Stop();

        stats.cubes_x = cubes_x;
        stats.cubes_y = cubes_y;
        stats.cubes_z = cubes_z;
        stats.allocation_time += allocation.Elapsed.TotalSeconds;
        stats.sampling_time += sampling.Elapsed.TotalSeconds;
        stats.flagging_time += flagging.Elapsed.TotalSeconds;
        stats.sample_points = grid_data_length;

        return new MTVTMesh();
    }
}

// TODO: different lattice structures
// TODO: different merging techniques
// TODO: time/memory tracking
// TODO: vertex/index count
