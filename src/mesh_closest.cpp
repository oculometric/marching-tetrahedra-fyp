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

// based on this https://github.com/ranjeethmahankali/galproject/blob/main/galcore/Mesh.cpp
void MappedMesh::closestPointOnTri(size_t triangle_ind, Vector3 test_point, float& best_sq_dist, Vector3& closest_point, float& best_sdf)
{
    const uint32_t i0 = indices[(triangle_ind * 3) + 0];
    const uint32_t i1 = indices[(triangle_ind * 3) + 1];
    const uint32_t i2 = indices[(triangle_ind * 3) + 2];
    const Vector3 v0 = vertices[i0];
    const Vector3 v1 = vertices[i1];
    const Vector3 v2 = vertices[i2];
    const Vector3 vs[3] = { v0, v1, v2 };

    const Vector3 norm = normals[triangle_ind];
    const Vector3 proj = norm * ((v0 - test_point) ^ norm);
    const float sq_dist = sq_mag(proj);
    if (sq_dist >= best_sq_dist)
        return;

    const Vector3 proj_point = test_point + proj;

    int num_failed = 0;
    for (int i = 0; i < 3; ++i)
    {
        const Vector3 va = vs[i];
        const Vector3 vb = vs[(i + 1) % 3];
        const bool is_outside = (((va - proj_point) % (vb - proj_point)) ^ norm) < 0.0f;
        if (is_outside)
        {
            ++num_failed;
            const Vector3 vl = vb - va;
            const float r = ::min(::max((vl ^ (proj_point - va)) / mag(vl), 0.0f), 1.0f);
            const Vector3 clamped_proj_point = (vb * r) + (va * (1.0f - r));
            const float clamped_sq_dist = sq_mag(clamped_proj_point - test_point);
            if (clamped_sq_dist < best_sq_dist)
            {
                best_sq_dist = clamped_sq_dist;
                closest_point = clamped_proj_point;
                best_sdf = (closest_point - test_point) ^ norm;
            }
        }
        if (num_failed > 1)
            break;
    }
    if (num_failed == 0)
    {
        best_sq_dist = sq_dist;
        closest_point = proj_point;
        best_sdf = (closest_point - test_point) ^ norm;
    }
}

float MappedMesh::closestPointSDF(MTVT::Vector3 vec)
{
    float best_sq_dist = INFINITY;
    Vector3 closest_point = Vector3{ 0, 0, 0 };
    float best_sdf = 0;

    for (size_t i = 0; i < indices.size() / 3; ++i)
    {
        closestPointOnTri(i, vec, best_sq_dist, closest_point, best_sdf);
    }

    return best_sdf;
}
