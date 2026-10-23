#ifndef BACKEND_H
#define BACKEND_H

#include <stdlib.h>
#include "star_data.h"

void multi_core_basic(StarData* Galaxy);
void single_core_basic(StarData* Galaxy);

void multi_core_simd(StarData* Galaxy);
void single_core_simd(StarData* Galaxy);

void gpu_openmp(StarData* Galaxy);
void hybrid(StarData* Galaxy);

#endif
