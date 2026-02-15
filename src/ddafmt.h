#pragma once

#include <vector>
#include <cstdint>

#include "Vector3.h"
#include "MTVT.h"

namespace MTVT
{

Mesh dynamicEvaluate(float (*sample_func)(Vector3), float threshold_value);

}
