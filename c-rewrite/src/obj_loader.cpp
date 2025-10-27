#include "obj_loader.h"

#include <fstream>

using namespace std;
using namespace MTVT;

// splits a formatted OBJ face corner into its component indices
static inline uint16_t splitOBJFaceCorner(string str)
{
    size_t first_break_ind = str.find('/');
    return static_cast<uint16_t>(stoi(str.substr(0, first_break_ind)) - 1);
}

bool readObj(std::string file_name, std::vector<Vector3>& vertices, std::vector<uint16_t>& indices)
{
    ifstream file;
    file.open(file_name);
    if (!file.is_open())
    {
        return false;
    }

    // vectors to load data into
    vector<Vector3> tmp_co;
    vector<Vector3> tmp_cl;
    vector<uint16_t> tmp_fc;

    // temporary locations for reading data to
    string tmps;
    Vector3 tmp3;

    vertices.clear();
    indices.clear();

    // repeat for every line in the file
    while (!file.eof())
    {
        file >> tmps;
        if (tmps == "v")
        {
            // read a vertex coordinate
            file >> tmp3.x;
            file >> tmp3.y;
            file >> tmp3.z;
            vertices.push_back(tmp3);
        }
        else if (tmps == "f")
        {
            // read a face (only supports triangles)
            file >> tmps;
            indices.push_back(splitOBJFaceCorner(tmps));
            file >> tmps;
            indices.push_back(splitOBJFaceCorner(tmps));
            file >> tmps;
            indices.push_back(splitOBJFaceCorner(tmps));
        }
        file.ignore(SIZE_MAX, '\n');
    }

    // transform from Z back Y up space into Z up Y forward space
    for (Vector3& fv : vertices)
        fv = Vector3{ fv.x, -fv.z, fv.y };

    return true;
}

