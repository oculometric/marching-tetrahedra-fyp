#pragma once

#include "Vector3.h"

float fbm_random(Vector3 coord);

float fbm_noise(Vector3 coord);

float fbm(Vector3 _coord, int _octaves, float _lacunarity, float _gain);