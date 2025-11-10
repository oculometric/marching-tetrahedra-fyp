#pragma once

#include "MTVT.h"

namespace MTVT
{

struct MeshStats
{
	double area_mean;
	double area_max;
	double area_min;
	double area_sd;

	double aspect_mean;
	double aspect_max;
	double aspect_min;
	double aspect_sd;
};

MeshStats compileStats(const Mesh& mesh);

}