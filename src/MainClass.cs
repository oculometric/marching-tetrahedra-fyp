using System.Numerics;

namespace mtvt
{
    class MainClass
    {
        static float sphereFunc(Vector3 v)
        {
            return (v - (Vector3.One * 3.0f)).Length() / 3.0f;
        }

        static void Main(string[] args)
        {
            MTVTBuilder builder = new MTVTBuilder();
            builder.configure(Vector3.Zero, Vector3.One * 6.0f, 0.05f, sphereFunc, 1.0f);
            MTVTDebugStats stats = new MTVTDebugStats();

            const int its = 100;

            for (int i = 0; i < its; ++i)
            {
                Console.WriteLine("test iteration {0}/{1}", i, its);
                Console.CursorTop--;
                builder.generate(ref stats);
            }

            Console.Beep();
            Console.WriteLine("-- summary -----------------------------");
            Console.WriteLine("  sphere test ({0} iterations)", its);
            Console.WriteLine("  {0}x{1}x{2} resolution", stats.cubes_x, stats.cubes_y, stats.cubes_z);
            Console.WriteLine("  results:");
            Console.WriteLine("    sample points:  {0}", stats.sample_points);
            Console.WriteLine("    edges:          {0}", stats.edges);
            Console.WriteLine("    tetrahedra:     {0}", stats.tetrahedra);
            Console.WriteLine("    vertices:       {0}", stats.vertices);
            Console.WriteLine("    indices:        {0}", stats.indices);
            Console.WriteLine("  performance:");
            Console.WriteLine("    allocation:     {0:0.000000}s", stats.allocation_time / its);
            Console.WriteLine("    sampling:       {0:0.000000}s", stats.sampling_time / its);
            Console.WriteLine("    flagging:       {0:0.000000}s", stats.flagging_time / its);
            Console.WriteLine("    vertex:         {0:0.000000}s", stats.vertex_time / its);
            Console.WriteLine("    geometry:       {0:0.000000}s", stats.geometry_time / its);
            Console.WriteLine("----------------------------------------");
        }
    }
}
