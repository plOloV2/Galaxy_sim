#include "star_data.h"
#include <omp.h>


#define PACKAGE_SIZE 64


void g_force_vec(
    _Float32 x1, _Float32 x2,
    _Float32 y1, _Float32 y2,
    _Float32 z1, _Float32 z2,
    _Float32 m1, _Float32 m2,
    _Float32* f_vec_x, _Float32* f_vec_y, _Float32* f_vec_z
);


void g_force_vec_avx2(
    _Float32 x1, const _Float32 x2[static 8],
    _Float32 y1, const _Float32 y2[static 8],
    _Float32 z1, const _Float32 z2[static 8],
    _Float32 m1, const _Float32 m2[static 8],
    _Float32* restrict f_vec_x,
    _Float32* restrict f_vec_y,
    _Float32* restrict f_vec_z
);


void multi_core_simd(StarData* Galaxy){

    #pragma omp parallel for schedule(static, PACKAGE_SIZE)
    for(uint32_t i = 0; i < Galaxy->N_stars; i++){

        _Float32 fx = 0.0f;
        _Float32 fy = 0.0f;
        _Float32 fz = 0.0f;

        _Float32 x1 = Galaxy->pos_x[i];
        _Float32 y1 = Galaxy->pos_y[i];
        _Float32 z1 = Galaxy->pos_z[i];
        _Float32 m1 = Galaxy->mass[i];

        uint32_t j = 0;

        /** Process 8 stars per iteration with AVX2. */
        for(; j + 8 <= Galaxy->N_stars; j += 8){

            g_force_vec_avx2(
                x1, &Galaxy->pos_x[j],
                y1, &Galaxy->pos_y[j],
                z1, &Galaxy->pos_z[j],
                m1, &Galaxy->mass[j],
                &fx, &fy, &fz
            );

        }

        /** Scalar tail loop if N_stars is not a multiple of 8. */
        for(; j < Galaxy->N_stars; j++){
            if(j == i)
                continue;

            g_force_vec(
                x1, Galaxy->pos_x[j],
                y1, Galaxy->pos_y[j],
                z1, Galaxy->pos_z[j],
                m1, Galaxy->mass[j],
                &fx, &fy, &fz
            );

        }

        _Float32 mass_over = 1.0f / m1;

        Galaxy->v_x[i] += fx * mass_over;
        Galaxy->v_y[i] += fy * mass_over;
        Galaxy->v_z[i] += fz * mass_over;

    }

    #pragma omp parallel for schedule(static, PACKAGE_SIZE)
    for(uint32_t i = 0; i < Galaxy->N_stars; i++){
        
        Galaxy->pos_x[i] += Galaxy->v_x[i];
        Galaxy->pos_y[i] += Galaxy->v_y[i];
        Galaxy->pos_z[i] += Galaxy->v_z[i];

    }

}
