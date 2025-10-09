using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

class MTVT
{
    public struct Mesh
    {
        List<Vector3> vertices;
        List<Vector3> normals;
        List<uint> indices;
    }

    private static void samplingPass(out float[] sample_points, Vector3 minimum_extent, Vector3 maximum_extent, float resolution)
    {
        // TODO: sampling pass - compute the values at all of the sample points
    }

    private static void flaggingPass(out int[] sample_prox_flags, in float[] sample_points, Vector3 grid_size)
    {
        // TODO: flagging pass - check all of the edges around each sample point, and set the edge flag bits
    }

    private static void vertexPass(out uint[] edge_indices, out List<Vector3> vertices, in float[] sample_points, in float[] sample_prox_flags)
    {
        // TODO: vertex pass - generate vertices for edges with flags set, and merge them where possible, assigning vertex references to these edges
    }

    private static void geometryPass(out List<uint> index_buffer, in uint[] edge_indices, in float[] sample_points, Vector3 grid_size)
    {
        // TODO: geometry pass - generate per-tetrahedron geometry from the edge/sample point info, discard triangles which zero size
    }

    // TODO: different lattice structures
    // TODO: different merging techniques
    // TODO: time/memory tracking
    // TODO: vertex/index count
    // TODO: summary generation tech

    public static Mesh computeGeometry(Vector3 minimum_extent, Vector3 maximum_extent, float resolution, Func<Vector3, float> sample_func)
    {
        // TODO: root func
        return new Mesh();
    }
}
