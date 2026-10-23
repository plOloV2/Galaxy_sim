#include "star_data.h"
#include "const.h"
#include <math.h>
#include <omp.h>

void hybrid(StarData* Galaxy){
    uint32_t N = Galaxy->N_stars;

    // Starts with an initial 75% GPU / 25% CPU split, then self-tunes every call
    static double gpu_ratio = 0.75;

    uint32_t split = (uint32_t)(N * gpu_ratio);
    if (split < 1) split = 1;
    if (split >= N) split = N - 1;
    uint32_t cpu_len = N - split;

    _Float32* pos_x = Galaxy->pos_x;
    _Float32* pos_y = Galaxy->pos_y;
    _Float32* pos_z = Galaxy->pos_z;
    _Float32* v_x   = Galaxy->v_x;
    _Float32* v_y   = Galaxy->v_y;
    _Float32* v_z   = Galaxy->v_z;
    _Float32* mass  = Galaxy->mass;

    double t_start = omp_get_wtime();
    double t_cpu_done = 0.0, t_gpu_done = 0.0;

    // 1. Fire GPU asynchronously on [0 : split) using 'nowait'
    #pragma omp target teams distribute parallel for nowait \
        map(to: pos_x[0:N], pos_y[0:N], pos_z[0:N], mass[0:N]) \
        map(tofrom: v_x[0:split], v_y[0:split], v_z[0:split])
    for (uint32_t i = 0; i < split; i++) {
        _Float32 local_fx = 0.0f, local_fy = 0.0f, local_fz = 0.0f;

        #pragma omp unroll partial(UNROL_INNER)
        for (uint32_t j = 0; j < N; j++) {
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

    // 2. Immediately run CPU SIMD on [split : N) while GPU works in the background
    #pragma omp parallel for schedule(guided, 64)
    for (uint32_t i = split; i < N; i++) {
        _Float32 local_fx = 0.0f, local_fy = 0.0f, local_fz = 0.0f;
        _Float32 x1 = pos_x[i], y1 = pos_y[i], z1 = pos_z[i], m1 = mass[i];

        #pragma omp simd reduction(+:local_fx, local_fy, local_fz)
        for (uint32_t j = 0; j < N; j++) {
            _Float32 dx = pos_x[j] - x1;
            _Float32 dy = pos_y[j] - y1;
            _Float32 dz = pos_z[j] - z1;

            _Float32 r = sqrtf(dx*dx + dy*dy + dz*dz + DIST_BARIER);
            _Float32 nr = 1.0f / r;
            _Float32 g = G_const * m1 * mass[j] * nr * nr * nr;

            local_fx += dx * g;
            local_fy += dy * g;
            local_fz += dz * g;
        }

        _Float32 mass_over = 1.0f / m1;
        v_x[i] += local_fx * mass_over;
        v_y[i] += local_fy * mass_over;
        v_z[i] += local_fz * mass_over;
    }
    t_cpu_done = omp_get_wtime() - t_start;

    // 3. Wait for the asynchronous GPU kernel and PCIe transfer to complete
    #pragma omp taskwait
    t_gpu_done = omp_get_wtime() - t_start;

    // 4. Dynamically re-balance split ratio for the next step
    // Note: if t_gpu_done <= t_cpu_done, the GPU finished before or with the CPU.
    double gpu_rate = (double)split / t_gpu_done;
    double cpu_rate = (double)cpu_len / t_cpu_done;
    double target_ratio = gpu_rate / (gpu_rate + cpu_rate);
    // Smooth update (exponential moving average) to avoid oscillation
    gpu_ratio = 0.5 * gpu_ratio + 0.5 * target_ratio;
    if (gpu_ratio < 0.05) gpu_ratio = 0.05;
    if (gpu_ratio > 0.98) gpu_ratio = 0.98;

    // 5. Update all positions
    #pragma omp parallel for schedule(guided, 64)
    for (uint32_t i = 0; i < N; i++) {
        pos_x[i] += v_x[i];
        pos_y[i] += v_y[i];
        pos_z[i] += v_z[i];
    }

}
