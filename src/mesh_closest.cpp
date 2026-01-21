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

void prepareOctree(OctreeNode& parent, int levels)
{
    if (levels <= 0)
        return;

    parent.is_leaf = false;
    parent.children.resize(8);

    int i = 0;
    for (auto& child : parent.children)
    {
        child.is_leaf = true;
        child.parent = &parent;
        child.half_extent = parent.half_extent / 2;
        switch (i)
        {
        case 0: child.center = child.parent->center + (child.half_extent * Vector3{ 1, 1, 1 }); break; // PXPYPZ = 0b000
        case 1: child.center = child.parent->center + (child.half_extent * Vector3{ -1, 1, 1 }); break; // NXPYPZ = 0b001
        case 2: child.center = child.parent->center + (child.half_extent * Vector3{ 1, -1, 1 }); break; // PXNYPZ = 0b010
        case 3: child.center = child.parent->center + (child.half_extent * Vector3{ -1, -1, 1 }); break; // NXNYPZ = 0b011
        case 4: child.center = child.parent->center + (child.half_extent * Vector3{ 1, 1, -1 }); break; // PXPYNZ = 0b100
        case 5: child.center = child.parent->center + (child.half_extent * Vector3{ -1, 1, -1 }); break; // NXPYNZ = 0b101
        case 6: child.center = child.parent->center + (child.half_extent * Vector3{ 1, -1, -1 }); break; // PXNYNZ = 0b110
        case 7: child.center = child.parent->center + (child.half_extent * Vector3{ -1, -1, -1 }); break; // NXNYNZ = 0b111
        }
        child.max = child.center + child.half_extent;
        child.min = child.center - child.half_extent;
        ++i;
        prepareOctree(child, levels - 1);
    }
}

void MappedMesh::buildOctree()
{
    Vector3 bmin = { INFINITY, INFINITY, INFINITY };
    Vector3 bmax = -bmin;

    for (const Vector3& v : vertices)
    {
        bmin = min(bmin, v);
        bmax = max(bmax, v);
    }

    octree.center = (bmin + bmax) / 2;
    octree.half_extent = (bmax - bmin) / 2;
    octree.max = bmax;
    octree.min = bmin;
    prepareOctree(octree, 2);

    octree.triangles.reserve(indices.size() / 3);
    for (size_t i = 0; i < indices.size() / 3; ++i)
        octree.triangles.push_back(i);
    sortTriangles(octree);
}

bool withinBounds(Vector3 point, Vector3 min, Vector3 max)
{
    if (point.x > max.x || point.y > max.y || point.z > max.z)
        return false;
    if (point.x < min.x || point.y < min.y || point.z < min.z)
        return false;
    return true;
}

void MappedMesh::sortTriangles(OctreeNode& node)
{
    if (node.is_leaf)
        return;

    while (!node.triangles.empty())
    {
        size_t triangle = node.triangles[node.triangles.size() - 1];
        node.triangles.pop_back();
        
        for (auto& child : node.children)
        {
            float best_sq_dist = INFINITY;
            Vector3 closest_point = Vector3{ 0, 0, 0 };
            float best_sdf = 0;
            closestPointOnTri(triangle, child.center, best_sq_dist, closest_point, best_sdf);
            if (withinBounds(closest_point, child.min, child.max))
                child.triangles.push_back(triangle);
        }
    }

    for (auto& child : node.children)
        sortTriangles(child);
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
    buildOctree();
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
    if (sq_mag(norm) < 0.0001f)
        return;
    const Vector3 proj = norm * ((v0 - test_point) ^ norm);
    const float sq_dist = sq_mag(proj);
    if (sq_dist >= best_sq_dist)
        return;

    const Vector3 proj_point = test_point + proj;

    int num_failed = 0;
    for (int i = 0; i < 3; ++i)
    {
        Vector3 va = vs[i];
        Vector3 vb = vs[(i + 1) % 3];
        bool is_outside = (((va - proj_point) % (vb - proj_point)) ^ norm) < 0.0f;
        if (is_outside)
        {
            ++num_failed;
            Vector3 vl = vb - va;
            float r = ::min(::max((vl ^ (proj_point - va)) / mag(vl), 0.0f), 1.0f);
            Vector3 clamped_proj_point = (vb * r) + (va * (1.0f - r));
            float clamped_sq_dist = sq_mag(clamped_proj_point - test_point);
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

void getLocator(const OctreeNode& node, Vector3 position, vector<int>& locator)
{
    if (node.is_leaf)
        return;

    int this_cell = 0;
    bool gx = position.x > node.center.x;
    bool gy = position.y > node.center.y;
    bool gz = position.z > node.center.z;
    this_cell = (gx ? 0b000 : 0b001)
        | (gy ? 0b000 : 0b010)
        | (gz ? 0b000 : 0b100);

    locator.push_back(this_cell);

    getLocator(node.children[this_cell], position, locator);
}

pair<vector<size_t>::iterator, vector<size_t>::iterator> getIterators(OctreeNode& octree, const vector<int>& locator)
{
    OctreeNode* node = &octree;
    for (int i : locator)
        node = &(node->children[i]);
    return { node->triangles.begin(), node->triangles.end() };
}

float MappedMesh::closestPointSDF(MTVT::Vector3 vec)
{
    float best_sq_dist = INFINITY;
    Vector3 closest_point = Vector3{ 0, 0, 0 };
    float best_sdf = 0;

    vector<int> locator;
    getLocator(octree, vec, locator);
    auto its = getIterators(octree, locator);

    // TODO: if no triangles are found, expand to search the 26 quadrants around it
    // TODO: actually search the 8 nearest quadrants i think.... for integrity
    for (auto it = its.first; it != its.second; ++it)
    {
        closestPointOnTri(*it, vec, best_sq_dist, closest_point, best_sdf);
    }
    //for (size_t i = 0; i < indices.size() / 3; ++i)
    //    closestPointOnTri(i, vec, best_sq_dist, closest_point, best_sdf);

    return best_sdf;
}
