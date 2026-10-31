#include <omp.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "backend.h"
#include "version.h"
#include "star_data.h"

bool run_test_mode(int size_count, char* size_args[], uint64_t seed);

StarData Galaxy; 

static bool parse_seed(const char* arg, uint64_t* out_seed){

    if(arg == nullptr || arg[0] == '\0' || arg[0] == '-')
        return false;

    errno = 0;
    char* endptr = nullptr;
    unsigned long long val = strtoull(arg, &endptr, 10);

    if(endptr == arg || *endptr != '\0' || errno == ERANGE || val == 0)
        return false;

    *out_seed = (uint64_t)val;
    return true;

}

int main(int argc, char* argv[]){

    #ifndef NDEBUG
        printf("Current version: %s\n", APP_VERSION);
    #endif

    bool test_mode = false;
    uint64_t seed = 0;
    int size_count = 0;

    for(int i = 1; i < argc; i++){

        if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0)
        {
            test_mode = true;
        }
        else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seed") == 0)
        {
            if(i + 1 >= argc)
            {
                fprintf(stderr, "ERROR: Missing seed value after '%s'.\n", argv[i]);
                return 1;
            }

            if(!parse_seed(argv[++i], &seed))
            {
                fprintf(stderr, "ERROR: Invalid seed value '%s' (must be an integer > 0).\n", argv[i]);
                return 1;
            }
        }
        else if(argv[i][0] == '-')
        {
            fprintf(stderr, "ERROR: Unknown option '%s'.\n", argv[i]);
            return 1;
        }
        else
        {
            argv[1 + size_count] = argv[i];
            size_count++;
        }

    }

    if(test_mode)
    {

        if(!run_test_mode(size_count, &argv[1], seed))
        {

            fprintf(stderr, "ERROR: Test mode execution failed.\n");
            return 1;

        }

        return 0;

    }
    else if(size_count > 0)
    {
        fprintf(stderr, "ERROR: Unexpected argument '%s' without -t/--test.\n", argv[1]);
        return 1;
    }

    printf("How big should the Galaxy be?\n");

    scanf("%u", &Galaxy.N_stars);

    printf("Generating random %u stars...", Galaxy.N_stars);

    if(!alloc_StarData(&Galaxy))
    {
        fprintf(stderr, "Allocation falied\n");
    }
    #ifndef NDEBUG
        printf("\nAlloc done.");
    #endif

    #ifndef NDEBUG
        if(seed != 0)
        {
            printf("Using user seed: %zu\n", seed);
        }
    #endif
    fill_StarData(&Galaxy, seed);

    printf("done\n\nWhich algorithm would you like to run:\n");
    printf("1. Single thread\n");
    printf("2. Single thread SIMD\n");
    printf("3. Multi thread\n");
    printf("4. Multi thread SIMD\n");
    printf("5. GPU CUDA\n");
    
    #ifdef OPENMP_GPU
        printf("6. GPU OpenMP (%s)\n", OPEN_MP_VENDOR);
    #endif

    printf("Your choice: ");

    uint32_t choice = 0;
    scanf("%u", &choice);


    void (*calculate)(StarData*) = &multi_core_basic;

    switch(choice){
        case 1:
            printf("\nYou have choosen Single threaded version.\n");
            calculate = &single_core_basic;
            break;

        case 2:
            printf("\nYou have choosen Single threaded with SIMD version.\n");
            calculate = &single_core_simd;
            break;

        case 3:
            printf("\nYou have choosen Multi threaded version.\n");
            calculate = &multi_core_basic;
            break;

        case 4:
            printf("\nYou have choosen Multi threaded with SIMD version.\n");
            calculate = &multi_core_simd;
            break;

        case 5:
            printf("\nYou have choosen GPU version.\n");
            // calculate = &single_core_simd;
            fprintf(stderr, "ERROR: Not implemented yet, running single thread version.");
            break;

        #ifdef OPENMP_GPU
        case 6:
            printf("\nYou have choosen OpenMP GPU version (%s).\n", OPEN_MP_VENDOR);
            calculate = &gpu_openmp;
            break;
        #endif

        default:
            fprintf(stderr, "ERROR: Incorect option choosen, closing program...");
            return 1;

    }
 

    bool stop = false;
    double time;

    while(!stop){

        time = omp_get_wtime();

        calculate(&Galaxy);

        time = omp_get_wtime() - time;

        if(time < 1.0)
        {
            printf("Calculations took: %.3lfms.\n", time*1000);
        } else
        {
            printf("Calculations took: %.3lfs.\n", time);
        }

        fflush(stdout);

    }

    return 0;

}
