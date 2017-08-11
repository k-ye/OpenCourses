// JAVA REFERENCE IMPLEMENTATION OF IMPROVED NOISE - COPYRIGHT 2002 KEN PERLIN.
// http://mrl.nyu.edu/~perlin/noise/
// translated to C++ for 6.837

#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <cmath>
#include <cstdio>

class Vector3f;

double perlinNoise(double x, double y, double z);
double perlinOctaveNoise(const Vector3f &pt, int octaves);

#endif
