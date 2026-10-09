#include "star_data.h"
#include "basic_CPU.h"
#include <string.h>
#include <omp.h>


void multi_core_basic(StarData* Galaxy){

    #pragma omp parallel for schedule(static, 64)
    for(uint32_t i = 0; i < Galaxy->N_stars; i++){

        Galaxy->f_vec_x[i] = 0.0f;
        Galaxy->f_vec_y[i] = 0.0f;
        Galaxy->f_vec_z[i] = 0.0f;

        for(uint32_t j = 0; j < Galaxy->N_stars; j++){

            if(j == i)
                continue;

            g_force_vec(Galaxy->pos_x[i], Galaxy->pos_x[j], Galaxy->pos_y[i], Galaxy->pos_y[j], Galaxy->pos_z[i], Galaxy->pos_z[j], Galaxy->mass[i], Galaxy->mass[j], &Galaxy->f_vec_x[i], &Galaxy->f_vec_y[i], &Galaxy->f_vec_z[i]);

        }

        _Float32 mass_over = 1.0f / Galaxy->mass[i];

        Galaxy->v_x[i] += Galaxy->f_vec_x[i] * mass_over;
        Galaxy->v_y[i] += Galaxy->f_vec_y[i] * mass_over;
        Galaxy->v_z[i] += Galaxy->f_vec_z[i] * mass_over;

    }

    #pragma omp parallel for schedule(static, 64)
    for(uint32_t i = 0; i < Galaxy->N_stars; i++){
        
        Galaxy->pos_x[i] += Galaxy->v_x[i];
        Galaxy->pos_y[i] += Galaxy->v_y[i];
        Galaxy->pos_z[i] += Galaxy->v_z[i];

    }

}
