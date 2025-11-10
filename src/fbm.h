#pragma once

#include "Vector3.h"

float fbm_random(MTVT::Vector3 coord);

float fbm_noise(MTVT::Vector3 coord);

float fbm(MTVT::Vector3 _coord, int _octaves, float _lacunarity, float _gain);