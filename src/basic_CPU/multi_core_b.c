#include "star_data.h"
#include <omp.h>


void g_force_vec(
    _Float32 x1, _Float32 x2,
    _Float32 y1, _Float32 y2,
    _Float32 z1, _Float32 z2,
    _Float32 m1, _Float32 m2,
    _Float32* f_vec_x, _Float32* f_vec_y, _Float32* f_vec_z
);


void multi_core_basic(StarData* Galaxy){

    #pragma omp parallel for schedule(guided, 64)
    for(uint32_t i = 0; i < Galaxy->N_stars; i++){

        _Float32 f_vec_x = 0.0f;
        _Float32 f_vec_y = 0.0f;
        _Float32 f_vec_z = 0.0f;

        for(uint32_t j = 0; j < Galaxy->N_stars; j++){

            if(j == i)
                continue;

            g_force_vec(
                Galaxy->pos_x[i], Galaxy->pos_x[j],
                Galaxy->pos_y[i], Galaxy->pos_y[j],
                Galaxy->pos_z[i], Galaxy->pos_z[j],
                Galaxy->mass[i], Galaxy->mass[j],
                &f_vec_x, &f_vec_y, &f_vec_z
            );

        }

        _Float32 mass_over = 1.0f / Galaxy->mass[i];

        Galaxy->v_x[i] += f_vec_x * mass_over;
        Galaxy->v_y[i] += f_vec_y * mass_over;
        Galaxy->v_z[i] += f_vec_z * mass_over;

    }

    #pragma omp parallel for schedule(guided, 64)
    for(uint32_t i = 0; i < Galaxy->N_stars; i++){
        
        Galaxy->pos_x[i] += Galaxy->v_x[i];
        Galaxy->pos_y[i] += Galaxy->v_y[i];
        Galaxy->pos_z[i] += Galaxy->v_z[i];

    }

}
