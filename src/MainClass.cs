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
            builder.configure(Vector3.Zero, Vector3.One * 6.0f, 2.0f, sphereFunc, 1.0f);
            builder.generate();
        }
    }
}
