#pragma once

#include <vector>
#include <string>

#include "Vector3.h"

struct OctreeNode
{
	MTVT::Vector3 center;
	MTVT::Vector3 half_extent;
	MTVT::Vector3 min;
	MTVT::Vector3 max;
	bool is_leaf = true;
	std::vector<OctreeNode> children;
	OctreeNode* parent = nullptr;
	std::vector<size_t> triangles;
};

class MappedMesh
{
private:
	// 1 per vertex
	std::vector<MTVT::Vector3> vertices;
	std::vector<std::vector<size_t>> vertex_uses;
	// 3 per triangle
	std::vector<uint16_t> indices;
	// 1 per triangle
	std::vector<MTVT::Vector3> normals;
	std::vector<MTVT::Vector3> centers;
	std::vector<std::pair<MTVT::Vector3, MTVT::Vector3>> edge_vectors;
	OctreeNode octree;

	void buildReverseIndexBuffer();
	void buildOctree();
	void sortTriangles(OctreeNode& node);
	void closestPointOnTri(size_t triangle_ind, MTVT::Vector3 test_point, float& best_sq_dist, MTVT::Vector3& closest_point, float& best_sdf);
	void closestPointInNode(const MTVT::Vector3& vec, const std::vector<int>& locator, float& best_sq_dist, MTVT::Vector3& closest_point, float& best_sdf);
	bool moveLocator(int direction, std::vector<int>& old_locator);

public:
	void load(std::string file);
	float closestPointSDF(MTVT::Vector3 vec);
};
