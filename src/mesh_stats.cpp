#include "mesh_stats.h"

#include <vector>

using namespace std;
using namespace MTVT;

MeshStats MTVT::compileStats(const Mesh& mesh)
{
    MeshStats stats{ };

    double area_sum = 0;
    stats.area_max = 0;
    stats.area_min = 0;
    vector<double> areas;   areas.reserve(mesh.indices.size() / 3);
    double aspect_sum = 0;
    stats.aspect_max = 0;
    stats.aspect_min = 0;
    vector<double> aspects; aspects.reserve(mesh.indices.size() / 3);

    for (size_t i = 0; i < mesh.indices.size() - 2; i += 3)
    {
        // compute area
        // compute aspect
        Vector3 v0 = mesh.vertices[mesh.indices[i]];
        Vector3 v1 = mesh.vertices[mesh.indices[i + 1]];
        Vector3 v2 = mesh.vertices[mesh.indices[i + 2]];
        
        Vector3 v01 = v1 - v0; double l01 = mag(v01);
        Vector3 v02 = v2 - v0; double l02 = mag(v02);

        double area = mag(v01 % v02) * 0.5f;
        double aspect = ::max(l01, l02) / ::min(l01, l02);
        area_sum += area;
        areas.push_back(area);
        if (area > stats.area_max) stats.area_max = area;
        if (area < stats.area_min) stats.area_min = area;
        aspect_sum += aspect;
        aspects.push_back(aspect);
        if (aspect > stats.aspect_max) stats.aspect_max = aspect;
        if (aspect < stats.aspect_min) stats.aspect_min = aspect;
    }

    size_t n = areas.size();
    stats.area_mean = area_sum / n;
    stats.aspect_mean = aspect_sum / n;

    double area_sq_dev_sum = 0;
    double aspect_sq_dev_sum = 0;

    for (size_t i = 0; i < n; i++)
    {
        double area_dev = (areas[i] - stats.area_mean);
        area_sq_dev_sum += area_dev * area_dev;
        double aspect_dev = (aspects[i] - stats.aspect_mean);
        aspect_sq_dev_sum += aspect_dev * aspect_dev;
    }

    stats.area_sd = sqrt(area_sq_dev_sum / n);
    stats.aspect_sd = sqrt(aspect_sq_dev_sum / n);

    return stats;
}
