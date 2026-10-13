#include <immintrin.h>
#include "const.h"

/** Horizontal sum of 8 _Float32 lanes in a __m256 register. */
static inline _Float32 hsum256_ps(__m256 v){

    __m128 vlow  = _mm256_castps256_ps128(v);
    __m128 vhigh = _mm256_extractf128_ps(v, 1);

    vlow = _mm_add_ps(vlow, vhigh);
    __m128 shuf = _mm_movehdup_ps(vlow);
    __m128 sums = _mm_add_ps(vlow, shuf);

    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);

    return (_Float32)_mm_cvtss_f32(sums);

}

/** Calculates gravity force vector for 1 star (x1, y1, z1, m1) from 8 stars (x2..m2). */
void g_force_vec_avx2(
    _Float32 x1, const _Float32 x2[static 8],
    _Float32 y1, const _Float32 y2[static 8],
    _Float32 z1, const _Float32 z2[static 8],
    _Float32 m1, const _Float32 m2[static 8],
    _Float32* restrict f_vec_x,
    _Float32* restrict f_vec_y,
    _Float32* restrict f_vec_z )
{

    /** Broadcast star1 parameters across 8 lanes. */
    __m256 vx1 = _mm256_set1_ps((float)x1);
    __m256 vy1 = _mm256_set1_ps((float)y1);
    __m256 vz1 = _mm256_set1_ps((float)z1);

    /** Load 8 star2 coordinates (unaligned load is equally fast on modern AVX2 when aligned). */
    __m256 vx2 = _mm256_loadu_ps((const float*)x2);
    __m256 vy2 = _mm256_loadu_ps((const float*)y2);
    __m256 vz2 = _mm256_loadu_ps((const float*)z2);

    /** Calculate distance vectors from star1 to 8 star2 instances. */
    __m256 vdx = _mm256_sub_ps(vx2, vx1);
    __m256 vdy = _mm256_sub_ps(vy2, vy1);
    __m256 vdz = _mm256_sub_ps(vz2, vz1);

    /** Calculate squared distance: dx*dx + dy*dy + dz*dz. */
    __m256 vdist2 = _mm256_mul_ps(vdx, vdx);
    vdist2 = _mm256_fmadd_ps(vdy, vdy, vdist2);
    vdist2 = _mm256_fmadd_ps(vdz, vdz, vdist2);

    /** Mask out self-interaction (where dx == 0, dy == 0, dz == 0). */
    __m256 valid_mask = _mm256_cmp_ps(vdist2, _mm256_setzero_ps(), _CMP_GT_OQ);

    /** r = sqrtf(dx*dx + dy*dy + dz*dz + DIST_BARIER) */
    __m256 vbarrier = _mm256_set1_ps((float)DIST_BARIER);
    __m256 vr = _mm256_sqrt_ps(_mm256_add_ps(vdist2, vbarrier));

    /** nr = 1.0f / r */
    __m256 vnr = _mm256_div_ps(_mm256_set1_ps(1.0f), vr);

    /** g = G_const * m1 * m2 * nr * nr * nr */
    __m256 vgm1 = _mm256_set1_ps((float)(G_const * m1));
    __m256 vm2  = _mm256_loadu_ps((const float*)m2);
    __m256 vnr3 = _mm256_mul_ps(_mm256_mul_ps(vnr, vnr), vnr);
    __m256 vg   = _mm256_mul_ps(_mm256_mul_ps(vgm1, vm2), vnr3);

    /** Zero out self-interaction lane if present. */
    vg = _mm256_and_ps(vg, valid_mask);

    /** Scale distance vectors by gravity. */
    vdx = _mm256_mul_ps(vdx, vg);
    vdy = _mm256_mul_ps(vdy, vg);
    vdz = _mm256_mul_ps(vdz, vg);

    /** Fuse vector with previous results. */
    *f_vec_x += hsum256_ps(vdx);
    *f_vec_y += hsum256_ps(vdy);
    *f_vec_z += hsum256_ps(vdz);
    
}
