#include "mesh_closest.h"

#include "obj_loader.h"

using namespace std;
using namespace MTVT;

void MappedMesh::buildReverseIndexBuffer()
{
    vertex_uses.resize(vertices.size());
    for (size_t i = 0; i < indices.size(); ++i)
        vertex_uses[indices[i]].push_back(i / 3);
}

void MappedMesh::load(string file)
{
    readObj(file, vertices, indices);
    for (size_t i = 0; i < indices.size() - 2; i += 3)
    {
        Vector3 v1 = vertices[indices[i]];
        Vector3 v2 = vertices[indices[i + 1]];
        Vector3 v3 = vertices[indices[i + 2]];

        Vector3 a = v2 - v1;
        Vector3 b = v3 - v1;
        edge_vectors.push_back({ a, b });
        normals.push_back(norm(a % b));
        centers.push_back((v1 + v2 + v3) / 3.0f);
    }

    buildReverseIndexBuffer();
}

float MappedMesh::closestPointSDF(MTVT::Vector3 vec)
{
    size_t closest_vertex = 0;
    float min_distance = INFINITY;
    Vector3 best_vert_to_vec;
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        Vector3 vert = vertices[i];
        float d_x = vert.x - vec.x;
        if (::abs(d_x) >= min_distance)
            continue;
        float d_y = vert.y - vec.y;
        if (::abs(d_y) >= min_distance)
            continue;
        float d_z = vert.z - vec.z;
        if (::abs(d_z) >= min_distance)
            continue;
        Vector3 vert_to_vec = { -d_x, -d_y, -d_z };
        float mag_v = mag(vert_to_vec);
        if (mag_v >= min_distance)
            continue;
        closest_vertex = i;
        best_vert_to_vec = vert_to_vec;
        min_distance = mag_v;
    }

    auto triangles_involved = vertex_uses[closest_vertex];
    float min_sdf = INFINITY;
    for (size_t triangle : triangles_involved)
    {
        Vector3 v0 = vertices[indices[(triangle * 3) + 0]];
        Vector3 v1 = vertices[indices[(triangle * 3) + 1]];
        Vector3 v2 = vertices[indices[(triangle * 3) + 2]];
        float sdf = (vec - v0) ^ normals[triangle];
        Vector3 P = vec - (normals[triangle] * sdf);
        
        // compute plane's normal
        Vector3 v0v1 = v1 - v0;
        Vector3 v0v2 = v2 - v0;
        // no need to normalize
        Vector3 N = v0v1 % v0v2; // N

        // Compute the intersection point P
        //Vector3 P = orig + dir * t;

        // Step 2: Calculate area of the triangle
        float area = mag(N) / 2; // Area of the full triangle

        // Step 3: Inside-outside test using barycentric coordinates
        Vector3 C;

        // Calculate u (for triangle BCP)
        Vector3 v1p = P - v1;
        Vector3 v1v2 = v2 - v1;
        C = v1v2 % v1p;
        float u = (mag(C) / 2) / area;
        if ((N ^ C) < 0) continue; // P is on the wrong side

        // Calculate v (for triangle CAP)
        Vector3 v2p = P - v2;
        Vector3 v2v0 = v0 - v2;
        C = v2v0 % v2p;
        float v = (mag(C) / 2) / area;
        if ((N ^ C) < 0) continue; // P is on the wrong side

        // Third edge
        Vector3 v0p = P - v0;
        // Vec3f v0v1 = v1 - v0; -> already defined
        C = v0v1 % v0p;
        if ((N ^ C) < 0) continue; // P is on the right side

        if (::abs(sdf) < ::abs(min_sdf))
            min_sdf = sdf;
    }
    return -min_sdf;
}
