#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "const.h"

/** Calculates gravity force vector for star1 from star2. */
void g_force_vec(_Float32 x1, _Float32 x2, _Float32 y1, _Float32 y2, _Float32 z1, _Float32 z2, _Float32 m1, _Float32 m2, _Float32* f_vec_x, _Float32* f_vec_y, _Float32* f_vec_z){

    /** Calculate vector of distance from star1 to star2. */
    _Float32 dx = x2 - x1;
    _Float32 dy = y2 - y1;
    _Float32 dz = z2 - z1;

    /** Calculate lenght of the vector (distance between). */
    _Float32 r = sqrtf(dx*dx + dy*dy + dz*dz + DIST_BARIER);

    /** Reverse distance to eliminate one costly division. */
    _Float32 nr = 1.0f / r;

    /** Calculate Newton's gravity force (new vecor lenght). */
    _Float32 g = G_const * m1 * m2 * nr * nr * nr;

    /** Scale our vector by the gravity. */
    dx *= g;
    dy *= g;
    dz *= g;

    /** Fuse vector with previouse resoults. */
    *f_vec_x += dx;
    *f_vec_y += dy;
    *f_vec_z += dz;

    return;

}
