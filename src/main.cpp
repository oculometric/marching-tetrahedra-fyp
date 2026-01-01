#include <iostream>
#include <string>
#include <format>
#include <fstream>
#include <chrono>

#include "MTVT.h"
#include "mesh_closest.h"
#include "benchmark.h"
#include "graphics.h"
#include "demo_functions.h"

using namespace std;
using namespace MTVT;

MappedMesh bunny_mesh;

// TODO: i should write an argument parsing library....

struct QueuedOperation
{
    bool is_mesh = false;
    string mesh_path;
    float(*function)(Vector3);
    string mesh_output_path = "output.obj";
    bool benchmark_enabled = false;
    string benchmark_output_path = "benchmark_yymmdd_hhmmss.csv";
    bool generate_animation = false;
    string animation_output_path = "output.gif";
    int iterations = 1;
    float resolution = 0.1f;
    Vector3 min_space = { -1, -1, -1 };
    Vector3 max_space = { 1, 1, 1 };
    float threshold = 0;
    Builder::LatticeType lattice = Builder::LatticeType::BODY_CENTERED_DIAMOND;
    Builder::ClusteringMode clustering = Builder::ClusteringMode::INTEGRATED;
};

int main(int cargs, char** vargs)
{
    vector<string> args;
    for (int i = 1; i < cargs; ++i)
        args.push_back(string(vargs[i]));

    bool start_gui = args.empty();

    // command line args
    // -h,--help -> show help
    // -g,--gui -> start GUI after function/mesh operations
    // -f,--function -> start function generation (requires string of three options)
    // -m,--mesh -> start mesh re-tesselation (requires string input of mesh path)
    // -b,--benchmark -> output benchmark data (optionally a string database path)
    // -o,--output -> enable output for mesh (optionally a string output path)
    // -i,--iterations -> set iterations count for function/mesh (requires int)
    // -r,--resolution -> set resolution for function/mesh (requires float)
    // -t,--threshold -> set threshold for function/mesh (requires float)
    // -p,--minimum -> set minimum bound for function/mesh (requires three floats)
    // -q,--maximum -> set maximum bound for function/mesh (requires three floats)
    // -l,--lattice -> set lattice type for function/mesh (requires string of two options)
    // -c,--clustering -> set clustering mode for function/mesh (requires string of three options)
    // -a,--animation -> generate animation from result of function/mesh (optionally an output path for the animation)
    
    // TODO: command line options
    vector<QueuedOperation> operations;

    for (auto arg_it = args.begin(); arg_it != args.end(); ++arg_it)
    {
        // TODO: support combos like '-bm' etc
        const string& arg = *arg_it;
        if (arg == "-g" || arg == "--gui")
            start_gui = true;
        else if (arg == "-h" || arg == "--help")
        {
            cout << "MTVT - Marching Tetrahedra Visualisation Tool - made by cassette costen" << endl;
            cout << "usage:" << endl;
            cout << "  -h,--help                                  show this help text" << endl;
            cout << "  -g,--gui                                   start the interactive GUI" << endl;
            cout << "  -m,--mesh PATH                             run a mesh re-tesselation of the OBJ file given by PATH (see below)" << endl;
            cout << "  -f,--function <SPHERE|FBM|BUMP>            run a tesselation of the specified builtin function (see below)" << endl;
            cout << endl;
            cout << "the following arguments are configuration options for generator commnds (--mesh or --function arguments)" << endl;
            cout << "and are only valid once at least one generator command has been specified. they will be applied to the" << endl;
            cout << "most recent generator command specified, can be stacked (in any order), and are all optional." << endl;
            cout << "  -b,--benchmark [PATH]                      collect benchmark stats and append them to the csv file given by PATH (or \'benchmark yy_mm_dd hh_mm_ss.csv\' if not provided)" << endl;
            cout << "  -o,--output [PATH]                         output the generated mesh to PATH (or \'output.obj\' if not provided)" << endl;
            cout << "  -a,--animation [PATH]                      record a turnaround animation of the generated mesh, and save it to PATH (or \'output.gif\' if not provided)" << endl;
            cout << endl;
            cout << "  -i,--iterations COUNT                      set the number of times to run the benchmark. only valid when using --benchmark" << endl;
            cout << "  -r,--resolution FLOAT                      set the size of the cubes used to march the sample space" << endl;
            cout << "  -p,--minimum FLOAT FLOAT FLOAT             set the minimum XYZ coordinates of the sample space" << endl;
            cout << "  -q,--maximum FLOAT FLOAT FLOAT             set the maximum XYZ coordinates of the sample space" << endl;
            cout << "  -t,--threshold FLOAT                       set the threshold value along which the isosurface should be computed" << endl;
            cout << "  -l,--lattice <BCDL|SIMPLE>                 set the lattice structure type to be used; must be one of the given options" << endl;
            cout << "  -c,--clustering <NONE|INTEGRATED|AFTER>    set the clustering mode to be used; must be one of the given options" << endl;
            cout << "examples:" << endl;
            // TODO: command line usage examples
        }
        else if (arg == "-m" || arg == "--mesh")
        {
            auto arg_next = arg_it + 1;
            if (arg_next == args.end())
            {
                cout << "error: mesh argument expects a path after it" << endl;
                return -1;
            }
            operations.push_back(QueuedOperation());
            operations[operations.size() - 1].is_mesh = true;
            operations[operations.size() - 1].mesh_path = *arg_next;
            ++arg_it;
        }
        else if (arg == "-f" || arg == "--function")
        {
            auto arg_next = arg_it + 1;
            if (arg_next == args.end())
            {
                cout << "error: function argument expects 'SPHERE', 'FBM', or 'BUMP' after it" << endl;
                return -1;
            }
            operations.push_back(QueuedOperation());
            operations[operations.size() - 1].is_mesh = false;
            if (*arg_next == "SPHERE")
                operations[operations.size() - 1].function = sphereFunc;
            else if (*arg_next == "FBM")
                operations[operations.size() - 1].function = fbmFunc;
            else if (*arg_next == "BUMP")
                operations[operations.size() - 1].function = bumpFunc;
            else
            {
                cout << "error: function argument expects 'SPHERE', 'FBM', or 'BUMP' after it" << endl;
                return -1;
            }
            ++arg_it;
        }
        else if (arg == "-b" || arg == "--benchmark")
        {

        }
    }

    if (start_gui)
    {
        GraphicsEnv graphics;
        graphics.create(1024, 1024);

        auto result = runBenchmark("realtime test", 1, { -1, -1, -1 }, { 1, 1, 1 }, 0.05f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
        graphics.setSummary(result.first);
        graphics.setMesh(result.second, { 0, 0, 0 });

        while (graphics.draw());

        graphics.destroy();
    }

    

    //auto result = runBenchmark("fbm", 50, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //printBenchmarkSummary(result.first);
    auto result = runBenchmark("sphere", 1, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, sphereFunc, 0.0f, Builder::SIMPLE_CUBIC, Builder::INTEGRATED, 8);
    dumpMeshToOBJ(result.second, "sphere_simple.obj");
    printBenchmarkSummary(result.first);

    

    //csv_file += runBenchmark("sphere", 10, { -2, -2, -2 }, { 2, 2, 2 }, 0.04f, sphereFunc, 1.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("sphere", 10, { -2, -2, -2 }, { 2, 2, 2 }, 0.04f, sphereFunc, 1.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //csv_file += runBenchmark("fbm", 10, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("fbm", 10, { -1, -1, -1 }, { 1, 1, 1 }, 0.02f, fbmFunc, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //csv_file += runBenchmark("bump", 10, { -4, -4, -4 }, { 4, 4, 4 }, 0.08f, [](Vector3 v) { return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z; }, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::NONE, 8);
    //csv_file += runBenchmark("bump", 10, { -4, -4, -4 }, { 4, 4, 4 }, 0.08f, [](Vector3 v) { return (1.0f / ((v.x * v.x) + (v.y * v.y) + 1)) - v.z; }, 0.0f, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    //
    /*bunny_mesh.load("res/stanford_bunny/bunny_touchup.obj");

    runBenchmark("bunny", 1, { -0.1f, -0.06f, -0.01f }, { 0.1f, 0.08f, 0.16f }, 0.04f, [](Vector3 v) { return bunny_mesh.closestPointSDF(v); }, 0.0F, Builder::BODY_CENTERED_DIAMOND, Builder::INTEGRATED, 8);
    */
    /*auto current_time = chrono::current_zone()->to_local(chrono::system_clock::now());
    string filename = format("out/benchmark_{0:%d_%m_%Y %H.%M.%S}.csv", current_time);
    ofstream csv(filename);
    if (!csv.is_open())
        return 1;
    csv << csv_file;
    csv.close();*/

    return 0;
}
