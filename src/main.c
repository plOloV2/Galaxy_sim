#include <omp.h>
#include <stdio.h>
#include "version.h"
#include "basic_CPU.h"
#include "star_data.h"

StarData Galaxy; 

int main(){

    #ifndef NDEBUG
        printf("Current version: %s\n", APP_VERSION);
    #endif

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

    printf("done\n");


    void (*calculate)(StarData*) = &multi_core_basic; 


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
