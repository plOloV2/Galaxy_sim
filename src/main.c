#include <stdio.h>
#include "version.h"
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

}
