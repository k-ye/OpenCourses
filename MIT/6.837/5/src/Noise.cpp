#include "Noise.h"
#include "PerlinNoise.h"
#include <iostream>
Noise::Noise() :
    octaves(0),
    init(false)
{
}

Noise::Noise(int _octaves,
             const Vector3f &color1, 
             const Vector3f &color2,
             float freq,
             float amp) :
    octaves(_octaves),
    frequency(freq),
    amplitude(amp)
{
    color[0] = color1;
    color[1] = color2;
    init = true;
}

Vector3f
Noise::getColor(const Vector3f &pos) const
{
    // IMPLEMENT: Fill in this function ONLY.
    // Interpolate between two colors by weighted average
    float M = sin(frequency * pos.x() + amplitude * perlinOctaveNoise(pos, octaves));
    M = M < 0 ? 0.0f : M;
    M = M > 1.0f ? 1.0f : M;
    return color[0] * M + color[1] * (1.0f - M);
}

bool
Noise::isValid() const
{
    return init;
}
