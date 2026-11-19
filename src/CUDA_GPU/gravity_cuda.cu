#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#define _Float32 float
extern "C" {
    #include "star_data.h"
    #include "const.h"
}

static constexpr float G_CONST = G_const;    
static constexpr float EPS = DIST_BARIER;

#undef _Float32 

#define BLOCK 256
#define TILE 256

#define CK(x) do { cudaError_t e_ = (x); if (e_ != cudaSuccess) { \
    fprintf(stderr, "CUDA error %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e_)); \
    exit(1); } } while (0)

//Naive way to calculate force
__global__ void naive_force_calc(uint32_t N,
    const float* __restrict__ x, const float* __restrict__ y,
        const float* __restrict__ z, const float* __restrict__ m,
        float* vx, float* vy, float* vz)
{
    uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    const float xi = x[i], yi = y[i], zi = z[i], mi = m[i];
    float fx = 0.f, fy = 0.f, fz = 0.f;

    for (uint32_t j = 0; j < N; j++)
    {
        float dx = x[j] - xi, dy = y[j] - yi, dz = z[j] - zi;
        float inv = rsqrtf(dx*dx + dy*dy + dz*dz + EPS);
        float g = G_CONST * mi * m[j] * inv * inv * inv;
        fx += dx * g;
        fy += dy * g;
        fz += dz * g;
    }

    const float inv_m = 1.f / mi;
    vx[i] += fx * inv_m;
    vy[i] += fy * inv_m;
    vz[i] += fz * inv_m;
}

//Shared memory force calculation
__global__ void tiled_force_calc(uint32_t N,
        const float* __restrict__ x, const float* __restrict__ y,
        const float* __restrict__ z, const float* __restrict__ m,
        float* vx, float* vy, float* vz)
{
    __shared__ float sx[TILE], sy[TILE], sz[TILE], sm[TILE];

    uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
    const bool valid = i < N;

    const float xi = valid ? x[i] : 0.f;
    const float yi = valid ? y[i] : 0.f;
    const float zi = valid ? z[i] : 0.f;
    const float mi = valid ? m[i] : 1.f;

    float fx = 0.f, fy = 0.f, fz = 0.f;

    for (uint32_t base = 0; base < N; base += TILE) {
        const uint32_t j = base + threadIdx.x;
        const bool in = j < N;
        sx[threadIdx.x] = in ? x[j] : 0.f;
        sy[threadIdx.x] = in ? y[j] : 0.f;
        sz[threadIdx.x] = in ? z[j] : 0.f;
        sm[threadIdx.x] = in ? m[j] : 0.f;
        __syncthreads();

        #pragma unroll 8
        for (int k = 0; k < TILE; k++) {
            float dx = sx[k] - xi, dy = sy[k] - yi, dz = sz[k] - zi;
            float inv = rsqrtf(dx*dx + dy*dy + dz*dz + EPS);
            float g = G_CONST * mi * sm[k] * inv * inv * inv;
            fx += dx * g; fy += dy * g; fz += dz * g;
        }
        __syncthreads();
    }

    if (valid) {
        const float inv_m = 1.f / mi;
        vx[i] += fx * inv_m;
        vy[i] += fy * inv_m;
        vz[i] += fz * inv_m;
    }
}

__global__ void update_body_pos(uint32_t N, float* x, float* y, float* z,
                                const float* vx, const float* vy, const float* vz)
{   
    uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) 
    {
        x[i] += vx[i];
        y[i] += vy[i];
        z[i] += vz[i];
    }
}   

static float g_kernel_ms = 0.f;

extern "C" void run(StarData* g)
{
    const uint32_t N = g->N_stars;
    const size_t bytes = (size_t)N * sizeof(float);

    //RAM (host)
    float* h[7] = { 
        g->pos_x, g->pos_y, g->pos_z, 
        g->v_x, g->v_y, g->v_z, g->mass };
    
    //DEVICE (GPU)
    // x y z vx vy vz m
    float* d[7];                   
    
    //Allocate memory on GPU and copy data from RAM to GPU
    for (int k = 0; k < 7; k++) {
        CK(cudaMalloc(&d[k], bytes));
        CK(cudaMemcpy(d[k], h[k], bytes, cudaMemcpyHostToDevice));
    }

    //GPU clock
    cudaEvent_t e0, e1;
    CK(cudaEventCreate(&e0));
    CK(cudaEventCreate(&e1));

    //How many threads in block
    const dim3 block(BLOCK);

    //How many blocks are needed
    const dim3 grid((N + BLOCK - 1) / BLOCK);

    //Calculate all forces for N bodies
    //and after that update positons 
    CK(cudaEventRecord(e0));
    tiled_force_calc<<<grid, block>>>(N, d[0], d[1], d[2], d[6], d[3], d[4], d[5]);
    update_body_pos<<<grid, block>>>(N, d[0], d[1], d[2], d[3], d[4], d[5]);
    CK(cudaEventRecord(e1));
    
    //Report any errors, wait till GPU finishes and save time
    CK(cudaGetLastError());
    CK(cudaEventSynchronize(e1));
    CK(cudaEventElapsedTime(&g_kernel_ms, e0, e1));

    //Copying position and forces to StarData without mass
    for (int k = 0; k < 6; k++)
        CK(cudaMemcpy(h[k], d[k], bytes, cudaMemcpyDeviceToHost));

    //Cleanup
    for (int k = 0; k < 7; k++) cudaFree(d[k]);
    cudaEventDestroy(e0);
    cudaEventDestroy(e1);
}

extern "C" {
    void gpu_cuda(StarData* g) { run(g); }
    double gpu_cuda_last_kernel_ms(void) { return (double)g_kernel_ms; }
} 