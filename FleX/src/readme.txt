1. scenes/wind.h


In main.cpp:

const Vec3 kWindDir = Vec3(3.0f, 0.0f, 15.0f); 
const float kNoise = Perlin1D(g_windTime*g_windFrequency, 10, 0.25f); 
Vec3 wind = g_windStrength*kWindDir*Vec3(kNoise, 0.0f, fabsf(kNoise));


2. scenes/rotate_table.h
3. scenes/occlusion.h
