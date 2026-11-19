#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "star_data.h"

void single_core_basic(StarData* Galaxy);
void gpu_cuda_naive(StarData* Galaxy);

static void make(StarData* g, uint32_t n, uint64_t seed) {
    g->N_stars = n;
    if (!alloc_StarData(g)) { fprintf(stderr, "alloc failed\n"); exit(1); }
    fill_StarData(g, seed);
}

/* Największy błąd względny |a-b| / max(1,|b|); ustawia *bad przy NaN/inf. */
static double cmp(const _Float32* a, const _Float32* b, uint32_t n, int* bad) {
    double m = 0.0;
    for (uint32_t i = 0; i < n; i++) {
        if (!isfinite((double)a[i])) *bad = 1;
        double d = fabs((double)a[i] - (double)b[i]) / fmax(1.0, fabs((double)b[i]));
        if (d > m) m = d;
    }
    return m;
}

int main(int argc, char** argv) {
    uint32_t n     = argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 1000;
    int      steps = argc > 2 ? atoi(argv[2]) : 1;
    uint64_t seed  = 69;
    if (n < 2 || steps < 1) { fprintf(stderr, "użycie: %s [N>=2] [kroki>=1]\n", argv[0]); return 1; }

    StarData cpu, gpu;
    make(&cpu, n, seed);
    make(&gpu, n, seed);

    printf("N=%u, kroki=%d\n", n, steps);
    for (int s = 1; s <= steps; s++) {
        single_core_basic(&cpu);
        gpu_cuda_naive(&gpu);

        int bad = 0;
        double ep = fmax(fmax(cmp(gpu.pos_x, cpu.pos_x, n, &bad), cmp(gpu.pos_y, cpu.pos_y, n, &bad)), cmp(gpu.pos_z, cpu.pos_z, n, &bad));
        double ev = fmax(fmax(cmp(gpu.v_x, cpu.v_x, n, &bad), cmp(gpu.v_y, cpu.v_y, n, &bad)), cmp(gpu.v_z, cpu.v_z, n, &bad));
        printf("krok %d: błąd pozycji %.2e, błąd prędkości %.2e %s\n", s, ep, ev, bad ? "[NaN/inf!]" : "");
    }

    if (n <= 8) {   // przy małym N można porównać liczby ręcznie
        for (uint32_t i = 0; i < n; i++)
            printf("star %u: CPU (%.5f %.5f %.5f)  GPU (%.5f %.5f %.5f)\n", i,
                   (double)cpu.pos_x[i], (double)cpu.pos_y[i], (double)cpu.pos_z[i],
                   (double)gpu.pos_x[i], (double)gpu.pos_y[i], (double)gpu.pos_z[i]);
    }
    return 0;
}