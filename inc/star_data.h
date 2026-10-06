#ifndef STAR_DATA_H
#define STAR_DATA_H

#include <stdint.h>

#define MAX_START_V    (_Float32)50
#define MAX_START_POS  (_Float32)100
#define MAX_START_MASS (_Float32)200

typedef struct {
    uint32_t N_stars;
    _Float32* pos_x;
    _Float32* pos_y;
    _Float32* pos_z;
    _Float32* v_x;
    _Float32* v_y;
    _Float32* v_z;
    _Float32* mass;
} StarData;

bool alloc_StarData(StarData* data);

void fill_StarData(StarData* data);

#endif 
