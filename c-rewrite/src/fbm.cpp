#include "fbm.h"

float fbm_random(Vector3 coord)
{
    float f = sin((coord ^ Vector3{ 12.98f, 78.23f, 35.63f })) * 43758.5f;
    return f - floor(f);
}

float fbm_noise(Vector3 coord)
{
    Vector3 flr = floor(coord);
    Vector3 frc = fract(coord);

    float tln = fbm_random(flr + Vector3{ 0, 0, 0 });
    float trn = fbm_random(flr + Vector3{ 1, 0, 0 });
    float bln = fbm_random(flr + Vector3{ 0, 1, 0 });
    float brn = fbm_random(flr + Vector3{ 1, 1, 0 });
    float tlf = fbm_random(flr + Vector3{ 0, 0, 1 });
    float trf = fbm_random(flr + Vector3{ 1, 0, 1 });
    float blf = fbm_random(flr + Vector3{ 0, 1, 1 });
    float brf = fbm_random(flr + Vector3{ 1, 1, 1 });

    Vector3 m = frc * frc * (Vector3{ 3.0f, 3.0f, 3.0f } - (Vector3{ 2.0f, 2.0f, 2.0f } *frc));

    float result =
        lerp(
            lerp(
                lerp(tln, trn, m.x),
                lerp(bln, brn, m.x),
                m.y
            ),
            lerp(
                lerp(tlf, trf, m.x),
                lerp(blf, brf, m.x),
                m.y
            ),
            m.z
        );

    return (result * 2.0f) - 1.0f;
}

float fbm(Vector3 _coord, int _octaves, float _lacunarity, float _gain)
{
    float amplitude = 1.0f;
    float frequency = 1.0f;

    float max_amplitude = 0.0f;

    float v = 0.0f;

    for (int i = 0; i < _octaves; i++)
    {
        v += fbm_noise(_coord * frequency) * amplitude;
        frequency *= _lacunarity;
        max_amplitude += amplitude;
        amplitude *= _gain;
    }

    v /= max_amplitude;

    return v;
}