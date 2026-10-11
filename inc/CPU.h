#ifndef BASIC_CPU_H
#define BASIC_CPU_H

#include <stdlib.h>
#include "star_data.h"

void multi_core_basic(StarData* Galaxy);
void single_core_basic(StarData* Galaxy);

void multi_core_simd(StarData* Galaxy);
void single_core_simd(StarData* Galaxy);

#endif
