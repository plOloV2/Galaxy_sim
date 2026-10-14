#include "star_data.h"
#include "const.h"
#include <math.h>
#include <omp.h>


#define UNROL_INNER 8 /** To be changed to check perf. */


/** 
 * Marks the function to be compiled for the GPU device.
 * 'static inline' guarantees no function call overhead inside the unrolled loop.
 */
#pragma omp declare target
static inline void g_force_vec_gpu(
    _Float32 x1, _Float32 x2,
    _Float32 y1, _Float32 y2,
    _Float32 z1, _Float32 z2,
    _Float32 m1, _Float32 m2,
    _Float32* f_vec_x, _Float32* f_vec_y, _Float32* f_vec_z)
{

    _Float32 dx = x2 - x1;
    _Float32 dy = y2 - y1;
    _Float32 dz = z2 - z1;

    _Float32 r = sqrtf(dx*dx + dy*dy + dz*dz + DIST_BARIER);
    _Float32 nr = 1.0f / r;
    _Float32 g = G_const * m1 * m2 * nr * nr * nr;

    *f_vec_x += dx * g;
    *f_vec_y += dy * g;
    *f_vec_z += dz * g;

}
#pragma omp end declare target


/**
 * GPU offloaded equivalent of gpu_openmp from multi_core_b.c
 */
void gpu_openmp(StarData* Galaxy){
    uint32_t N = Galaxy->N_stars;
    
    // Extract pointers locally to allow OpenMP to map array sections correctly.
    _Float32* pos_x = Galaxy->pos_x;
    _Float32* pos_y = Galaxy->pos_y;
    _Float32* pos_z = Galaxy->pos_z;
    
    _Float32* v_x = Galaxy->v_x;
    _Float32* v_y = Galaxy->v_y;
    _Float32* v_z = Galaxy->v_z;
    
    _Float32* mass = Galaxy->mass;

    // Map data to the GPU memory space.
    #pragma omp target data \
        map(tofrom: pos_x[0:N], pos_y[0:N], pos_z[0:N], v_x[0:N], v_y[0:N], v_z[0:N]) \
        map(to: mass[0:N])
    {
        
        // Offload outer loop to GPU thread teams
        #pragma omp target teams distribute parallel for
        for(uint32_t i = 0; i < N; i++){
            
            _Float32 local_fx = 0.0f;
            _Float32 local_fy = 0.0f;
            _Float32 local_fz = 0.0f;

            #pragma omp unroll partial(UNROL_INNER) 
            for(uint32_t j = 0; j < N; j++){

                g_force_vec_gpu(
                    pos_x[i], pos_x[j],
                    pos_y[i], pos_y[j],
                    pos_z[i], pos_z[j],
                    mass[i], mass[j],
                    &local_fx, &local_fy, &local_fz
                );

            }

            _Float32 mass_over = 1.0f / mass[i];

            v_x[i] += local_fx * mass_over;
            v_y[i] += local_fy * mass_over;
            v_z[i] += local_fz * mass_over;

        }

        // Offload position update loop to GPU thread teams
        #pragma omp target teams distribute parallel for
        for(uint32_t i = 0; i < N; i++){

            pos_x[i] += v_x[i];
            pos_y[i] += v_y[i];
            pos_z[i] += v_z[i];

        }

    }

}
