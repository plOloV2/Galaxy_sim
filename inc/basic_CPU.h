#ifndef BASIC_CPU_H
#define BASIC_CPU_H

#include <stdlib.h>

void g_force_vec(_Float32 x1, _Float32 x2, _Float32 y1, _Float32 y2, _Float32 z1, _Float32 z2, _Float32 m1, _Float32 m2, _Float32* f_vec_x, _Float32* f_vec_y, _Float32* f_vec_z);

void multi_core_basic(StarData* Galaxy);
void single_core_basic(StarData* Galaxy);

#endif
