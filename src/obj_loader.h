#pragma once

#include <string>
#include <vector>

#include "Vector3.h"

bool readObj(std::string file_name, std::vector<MTVT::Vector3>& vertices, std::vector<uint16_t>& indices);