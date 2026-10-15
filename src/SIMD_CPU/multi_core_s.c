#include "star_data.h"
#include "const.h"
#include <math.h>
#include <omp.h>


void multi_core_simd(StarData* Galaxy){
    uint32_t N = Galaxy->N_stars;

    
    #pragma omp parallel for schedule(guided, 64)
    for(uint32_t i = 0; i < N; i++){
        
        // Local accumulators 
        _Float32 local_fx = 0.0f;
        _Float32 local_fy = 0.0f;
        _Float32 local_fz = 0.0f;

        _Float32 x1 = Galaxy->pos_x[i];
        _Float32 y1 = Galaxy->pos_y[i];
        _Float32 z1 = Galaxy->pos_z[i];
        _Float32 m1 = Galaxy->mass[i];

        // Tell the CPU compiler to explicitly SIMD vectorize this branchless loop
        #pragma omp simd reduction(+:local_fx, local_fy, local_fz)
        for(uint32_t j = 0; j < N; j++){

            _Float32 dx = Galaxy->pos_x[j] - x1;
            _Float32 dy = Galaxy->pos_y[j] - y1;
            _Float32 dz = Galaxy->pos_z[j] - z1;

            _Float32 r = sqrtf(dx*dx + dy*dy + dz*dz + DIST_BARIER);
            _Float32 nr = 1.0f / r;
            _Float32 g = G_const * m1 * Galaxy->mass[j] * nr * nr * nr;

            local_fx += dx * g;
            local_fy += dy * g;
            local_fz += dz * g;

        }

        _Float32 mass_over = 1.0f / m1;
        Galaxy->v_x[i] += local_fx * mass_over;
        Galaxy->v_y[i] += local_fy * mass_over;
        Galaxy->v_z[i] += local_fz * mass_over;

    }

    #pragma omp parallel for schedule(guided, 64)
    for(uint32_t i = 0; i < N; i++){

        Galaxy->pos_x[i] += Galaxy->v_x[i];
        Galaxy->pos_y[i] += Galaxy->v_y[i];
        Galaxy->pos_z[i] += Galaxy->v_z[i];
    }

}
