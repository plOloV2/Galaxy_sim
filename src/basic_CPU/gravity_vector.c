#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define G_const (_Float32)(6.674e-11)

_Float32 g_force(_Float32 r, _Float32 m1, _Float32 m2){

    if(r == 0.0)
    {
        #ifndef NDEBUG
            fprintf(stderr, "ERROR: Distance equal 0, adding small const.\n");
        #endif

        r = 1.0e-13;
    }

    return (G_const * m1 * m2) / (r * r);

}

_Float32 distance(_Float32 x1, _Float32 x2, _Float32 y1, _Float32 y2, _Float32 z1, _Float32 z2){

    _Float32 dx = x1 - x2;
    _Float32 dy = y1 - y2;
    _Float32 dz = z1 - z2;

    return (_Float32)sqrtf(dx*dx + dy*dy + dz*dz);

}

