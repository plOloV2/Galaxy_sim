#include <omp.h>
#include <stdio.h>
#include <string.h>
#include "backend.h"
#include "version.h"
#include "star_data.h"

bool run_test_mode(int size_count, char* size_args[]);

StarData Galaxy; 

int main(int argc, char* argv[]){

    #ifndef NDEBUG
        printf("Current version: %s\n", APP_VERSION);
    #endif

    if(argc > 1 && strcmp(argv[1], "test") == 0)
    {

        if(!run_test_mode(argc - 2, &argv[2]))
        {

            fprintf(stderr, "ERROR: Test mode execution failed.\n");
            return 1;

        }

        return 0;

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

    fill_StarData(&Galaxy);

    printf("done\n\nWhich algorithm would you like to run:\n");
    printf("1. Single thread\n");
    printf("2. Single thread SIMD\n");
    printf("3. Multi thread\n");
    printf("4. Multi thread SIMD\n");
    printf("5. GPU CUDA\n");
    
    #ifdef OPENMP_GPU
        printf("6. GPU OpenMP (%s)\n", OPEN_MP_VENDOR);
        printf("7. Hybrid OpenMP CPU + GPU (%s)\n", OPEN_MP_VENDOR);
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

        case 7:
            printf("\nYou have choosen OpenMP Hybrid CPU + GPU version (%s).\n", OPEN_MP_VENDOR);
            calculate = &hybrid;
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
