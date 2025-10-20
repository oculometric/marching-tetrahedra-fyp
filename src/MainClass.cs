using System.Numerics;

namespace mtvt
{
    class MainClass
    {
        static float sphereFunc(Vector3 v)
        {
            return (v - (Vector3.One * 3.0f)).Length() / 3.0f;
        }

        static void runBenchmark(string name, int iterations, Vector3 min, Vector3 max, float cube_size, Func<Vector3, float> sampler, float threshold)
        {
            MTVTBuilder builder = new MTVTBuilder();
            builder.configure(min, max, cube_size, sampler, threshold);
            MTVTDebugStats stats = new MTVTDebugStats();

            for (int i = 0; i < iterations; ++i)
            {
                Console.WriteLine("{0} test iteration {1}/{2}", name, i, iterations);
                Console.CursorTop--;
                builder.generate(ref stats);
            }

            Console.Beep();
            Console.WriteLine("-- summary -----------------------------");
            Console.WriteLine("  {0} test ({1} iterations)", name, iterations);
            Console.WriteLine("  {0}x{1}x{2} resolution", stats.cubes_x, stats.cubes_y, stats.cubes_z);
            Console.WriteLine("  results:");
            Console.WriteLine("    sample points:  {0}", stats.sample_points);
            Console.WriteLine("    edges:          {0}", stats.edges);
            Console.WriteLine("    tetrahedra:     {0}", stats.tetrahedra);
            Console.WriteLine("    vertices:       {0}", stats.vertices);
            Console.WriteLine("    indices:        {0}", stats.indices);
            Console.WriteLine("  performance:");
            Console.WriteLine("    allocation:     {0:0.000000}s", stats.allocation_time / iterations);
            Console.WriteLine("    sampling:       {0:0.000000}s", stats.sampling_time / iterations);
            Console.WriteLine("    flagging:       {0:0.000000}s", stats.flagging_time / iterations);
            Console.WriteLine("    vertex:         {0:0.000000}s", stats.vertex_time / iterations);
            Console.WriteLine("    geometry:       {0:0.000000}s", stats.geometry_time / iterations);
            Console.WriteLine("----------------------------------------");
            Console.WriteLine();
        }

        static void Main(string[] args)
        {
            runBenchmark("sphere", 100, Vector3.Zero, Vector3.One * 6.0f, 0.06f, sphereFunc, 1.0f);
        }
    }
}
