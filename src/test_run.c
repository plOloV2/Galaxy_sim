#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "backend.h"
#include "star_data.h"


#define DEFAULT_TEST_RUNS 100U
#define EPSILON_REL  (_Float32)(1e-4f)
#define EPSILON_ABS  (_Float32)(1e-5f)

#define PASSES_CSV_FILE  "benchmark_passes.csv"
#define SUMMARY_CSV_FILE "benchmark_summary.csv"


typedef struct {
    const char* name;
    void (*calculate)(StarData*);
} AlgorithmEntry;


static const AlgorithmEntry ALGORITHMS[] = {
    {"basic_cpu",         &single_core_basic},
    {"single_core_simd",  &single_core_simd},
    {"multi_core_basic",  &multi_core_basic},
    {"multi_core_simd",   &multi_core_simd},
#ifdef OPENMP_GPU
    {"gpu_openmp",        &gpu_openmp},
#endif
#ifdef CUDA_GPU
    {"gpu_cuda",          &},
#endif
};


#define NUM_ALGORITHMS (sizeof(ALGORITHMS) / sizeof(ALGORITHMS[0]))


/** Frees all tables inside a StarData structure. */
static void free_StarData(StarData* data){
    if(data == nullptr)
        return;

    _Float32** table[NUM_TABLES] = {
        &data->pos_x, &data->pos_y, &data->pos_z,
        &data->v_x,   &data->v_y,   &data->v_z,
        &data->mass
    };

    for(size_t i = 0; i < NUM_TABLES; i++){

        free(*table[i]);
        *table[i] = nullptr;

    }

}


/** Copies all 7 star tables from src to dst (both must be allocated with the same N_stars). */
static void copy_StarData(StarData* dst, const StarData* src){

    size_t bytes = src->N_stars * sizeof(_Float32);

    _Float32* dst_table[NUM_TABLES] = {
        dst->pos_x, dst->pos_y, dst->pos_z,
        dst->v_x,   dst->v_y,   dst->v_z,
        dst->mass
    };

    const _Float32* src_table[NUM_TABLES] = {
        src->pos_x, src->pos_y, src->pos_z,
        src->v_x,   src->v_y,   src->v_z,
        src->mass
    };

    for(size_t i = 0; i < NUM_TABLES; i++){

        memcpy(dst_table[i], src_table[i], bytes);

    }

}


#ifndef NDEBUG
/**
 * Compares candidate results against reference basic_cpu results using:
 * |a - b| <= EPSILON_ABS + EPSILON_REL * max(|a|, |b|)
 */
[[nodiscard]] static bool verify_StarData(const StarData* ref, const StarData* candidate) {
    if(ref->N_stars != candidate->N_stars)
        return false;
    

    const _Float32* ref_table[NUM_TABLES] = {
        ref->pos_x, ref->pos_y, ref->pos_z,
        ref->v_x,   ref->v_y,   ref->v_z,
        ref->mass
    };

    const _Float32* cand_table[NUM_TABLES] = {
        candidate->pos_x, candidate->pos_y, candidate->pos_z,
        candidate->v_x,   candidate->v_y,   candidate->v_z,
        candidate->mass
    };

    for(size_t t = 0; t < NUM_TABLES; t++){
        for(size_t i = 0; i < ref->N_stars; i++){

            float a = (float)ref_table[t][i];
            float b = (float)cand_table[t][i];

            if(isnan(a) || isnan(b) || isinf(a) || isinf(b))
                return false;
            

            _Float32 diff   = (_Float32)fabsf(a - b);
            _Float32 max_ab = (_Float32)fmaxf(fabsf(a), fabsf(b));
            _Float32 limit  = EPSILON_ABS + EPSILON_REL * max_ab;

            if(diff > limit)
                return false;
            
        }

    }

    return true;

}
#endif


/** Parses and validates galaxy sizes from command-line arguments. */
[[nodiscard]] static bool parse_sizes(int size_count, char* size_args[], uint32_t* out_sizes){

    for(int i = 0; i < size_count; i++){

        char* endptr = nullptr;
        unsigned long val = strtoul(size_args[i], &endptr, 10);

        if(endptr == size_args[i] || *endptr != '\0' || val < 2 || val > UINT32_MAX)
        {

            #ifndef NDEBUG
                fprintf(stderr, "ERROR: Invalid Galaxy size argument '%s' (must be >= 2).\n", size_args[i]);
            #endif
            return false;

        }

        out_sizes[i] = (uint32_t)val;

    }

    return true;

}


bool run_test_mode(int size_count, char* size_args[]){

    if(size_count < 1)
    {

        fprintf(stderr, "ERROR: No Galaxy sizes provided after 'test'.\n");
        return false;

    }

    uint32_t* sizes = malloc((size_t)size_count * sizeof(uint32_t));
    if(sizes == nullptr)
    {

        #ifndef NDEBUG
            fprintf(stderr, "ERROR: Alloc of sizes array failed.\n");
        #endif
        return false;

    }

    if(!parse_sizes(size_count, size_args, sizes))
    {

        free(sizes);
        return false;

    }

    uint32_t num_runs = DEFAULT_TEST_RUNS;
    #ifndef NDEBUG
        bool check_results = false;
        printf("[DEBUG] Enter number of test runs: ");
        if(scanf("%u", &num_runs) != 1 || num_runs == 0)
        {

            fprintf(stderr, "ERROR: Invalid number of test runs.\n");
            free(sizes);
            return false;

        }

        uint32_t check_choice = 0;
        printf("[DEBUG] Check results against basic_cpu? (1 = Yes, 0 = No): ");
        if(scanf("%u", &check_choice) != 1 || (check_choice != 0 && check_choice != 1))
        {

            fprintf(stderr, "ERROR: Invalid choice for result check.\n");
            free(sizes);
            return false;

        }

        check_results = (check_choice == 1);
    #endif

    // Generate initial StarData for all given Galaxy sizes upfront
    StarData* datasets = malloc((size_t)size_count * sizeof(StarData));
    if(datasets == nullptr)
    {

        #ifndef NDEBUG
            fprintf(stderr, "ERROR: Alloc of datasets array failed.\n");
        #endif
        free(sizes);
        return false;

    }

    for(int i = 0; i < size_count; i++){

        datasets[i].N_stars = sizes[i];
        printf("Generating random %u stars (%d/%d)...", datasets[i].N_stars, i + 1, size_count);
        fflush(stdout);

        if(!alloc_StarData(&datasets[i]))
        {

            fprintf(stderr, "ERROR: Allocation of StarData for size %u failed.\n", sizes[i]);

            for(int j = 0; j < i; j++){

                free_StarData(&datasets[j]);

            }

            free(datasets);
            free(sizes);
            return false;

        }

        fill_StarData(&datasets[i]);
        printf("done\n");

    }

    FILE* passes_fp = fopen(PASSES_CSV_FILE, "w");
    if(passes_fp == nullptr)
    {

        #ifndef NDEBUG
            fprintf(stderr, "ERROR: Could not open %s for writing.\n", PASSES_CSV_FILE);
        #endif

        for(int i = 0; i < size_count; i++){

            free_StarData(&datasets[i]);

        }

        free(datasets);
        free(sizes);
        return false;

    }

    FILE* summary_fp = fopen(SUMMARY_CSV_FILE, "w");
    if(summary_fp == nullptr)
    {

        #ifndef NDEBUG
            fprintf(stderr, "ERROR: Could not open %s for writing.\n", SUMMARY_CSV_FILE);
        #endif
        fclose(passes_fp);

        for (int i = 0; i < size_count; i++){

            free_StarData(&datasets[i]);

        }

        free(datasets);
        free(sizes);
        return false;
        
    }

    double* run_times = malloc(num_runs * sizeof(double));
    if(run_times == nullptr)
    {

        #ifndef NDEBUG
            fprintf(stderr, "ERROR: Alloc of run_times failed.\n");
        #endif
        fclose(summary_fp);
        fclose(passes_fp);

        for(int i = 0; i < size_count; i++){

            free_StarData(&datasets[i]);
            
        }

        free(datasets);
        free(sizes);
        return false;

    }

    // Write CSV headers
    fprintf(passes_fp, "Galaxy_Size,Algorithm,Pass,Time_s,Check_Status\n");
    fprintf(summary_fp, "Galaxy_Size,Algorithm,Runs,Average_Time_s,Std_Dev_s\n");

    // Run benchmarks for each generated dataset
    for(int s = 0; s < size_count; s++){

        uint32_t current_size = datasets[s].N_stars;

        StarData work_data = {.N_stars = current_size};
        if(!alloc_StarData(&work_data))
        {

            fprintf(stderr, "ERROR: Alloc of working StarData failed.\n");
            free(run_times);
            fclose(summary_fp);
            fclose(passes_fp);

            for(int i = 0; i < size_count; i++){

                free_StarData(&datasets[i]);

            }

            free(datasets);
            free(sizes);
            return false;

        }

        #ifndef NDEBUG
        StarData ref_data = {.N_stars = current_size};
        if(check_results)
        {

            if(!alloc_StarData(&ref_data))
            {

                fprintf(stderr, "ERROR: Alloc of reference StarData failed.\n");
                free_StarData(&work_data);
                free(run_times);
                fclose(summary_fp);
                fclose(passes_fp);
                for (int i = 0; i < size_count; i++) {
                    free_StarData(&datasets[i]);
                }
                free(datasets);
                free(sizes);
                return false;
            }

        }
        #endif

        printf("\nRunning test on %u stars...\n", current_size);

        for(size_t a = 0; a < NUM_ALGORITHMS; a++){

            // Reset working data to the initial dataset for this galaxy size
            copy_StarData(&work_data, &datasets[s]);

            #ifndef NDEBUG
            if(check_results && a > 0)
                copy_StarData(&ref_data, &datasets[s]);
            
            #endif

            double sum_time = 0.0;

            printf("Testing %-18s : 0/%u (0.0%%) done",  ALGORITHMS[a].name, num_runs);
            fflush(stdout);

            for(uint32_t r = 0; r < num_runs; r++){

                double elapsed = omp_get_wtime();
                ALGORITHMS[a].calculate(&work_data);
                elapsed = omp_get_wtime() - elapsed;

                run_times[r] = elapsed;
                sum_time += elapsed;

                const char* status_str = "DISABLED";

                #ifndef NDEBUG
                if(check_results)
                {
                    bool passed = true;

                    if(a > 0)
                    {

                        // Advance reference implementation by one step and compare
                        single_core_basic(&ref_data);
                        passed = verify_StarData(&ref_data, &work_data);

                    }

                    if(!passed)
                    {

                        status_str = "FAILED";

                    }else
                    {

                        status_str = "OK";

                    }
                }
                #endif

                fprintf(passes_fp, "%u,%s,%u,%.9lf,%s\n",  current_size, ALGORITHMS[a].name, r + 1, elapsed, status_str);

                uint32_t completed = r + 1;
                double progress_pct = ((double)completed / (double)num_runs) * 100.0;

                printf("\rTesting %-18s : %u/%u (%.1f%%) done", ALGORITHMS[a].name, completed, num_runs, progress_pct);
                fflush(stdout);

            }

            printf("\n");

            // Compute average and standard deviation
            double avg_time = sum_time / (double)num_runs;
            double variance_sum = 0.0;

            for(uint32_t r = 0; r < num_runs; r++){

                double diff = run_times[r] - avg_time;
                variance_sum += diff * diff;

            }

            double std_dev = (num_runs > 1) ? sqrt(variance_sum / (double)(num_runs - 1)) : 0.0;

            fprintf(summary_fp, "%u,%s,%u,%.9lf,%.9lf\n", current_size, ALGORITHMS[a].name, num_runs, avg_time, std_dev);
        }

        #ifndef NDEBUG
        if (check_results)
            free_StarData(&ref_data);
        
        #endif

        free_StarData(&work_data);

    }

    free(run_times);
    fclose(summary_fp);
    fclose(passes_fp);

    for(int i = 0; i < size_count; i++)
        free_StarData(&datasets[i]);
    
    free(datasets);
    free(sizes);

    printf("\n\nBenchmark finished. Results written to '%s' and '%s'.\n", PASSES_CSV_FILE, SUMMARY_CSV_FILE);

    return true;

}
