#include <stdlib.h>
#include <stdio.h>
#include "star_data.h"
#include "Xoshiro256.h"

bool alloc_StarData(StarData* data){
    
    if(data->N_stars < 2)
        return false;


    _Float32** table[7] = {&data->pos_x, &data->pos_y, &data->pos_z, &data->v_x, &data->v_y, &data->v_z, &data->mass};

    #ifndef NDEBUG
        const char* names[7] = {"pos_x", "pos_y", "pos_z", "v_x", "v_y", "v_z", "mass"};
    #endif

    size_t bytes_to_alloc = data->N_stars * sizeof(_Float32);

    for(size_t i = 0; i < 7; i++) {
        *table[i] = malloc(bytes_to_alloc);
        
        if(*table[i] == NULL) {

            #ifndef NDEBUG
                fprintf(stderr, "ERROR: Alloc of %s failed.\n", names[i]);
            #endif

            // Free all previously successful allocations to prevent memory leaks
            for(size_t j = 0; j < i; j++) {
                free(*table[j]);
                *table[j] = NULL;
            }

            return false;
        }
        
    }

    return true;

}

typedef union{
    uint64_t input;
    uint32_t output[2];
} Conv;

void fill_StarData(StarData* data){

    xoshiro256_state* Xos = xoshiro_init();
    if(Xos == NULL){

        #ifndef NDEBUG
            fprintf(stderr, "ERROR: Alloc of xoshiro256_state failed.\n");
        #endif

        return;

    }


    _Float32* table[7] = {data->pos_x, data->pos_y, data->pos_z, data->v_x, data->v_y, data->v_z, data->mass};
    Conv raw_data;
    _Float32 scaler = MAX_START_POS;

    for(size_t i = 0; i < 7; i++){

        size_t iter = 0;

        while(iter < (data->N_stars - 1)) {

            raw_data.input = xoshiro_next(Xos);

            table[i][iter] = ((_Float32)raw_data.output[0] / (_Float32)UINT32_MAX) * scaler;
            table[i][iter + 1] = ((_Float32)raw_data.output[1] / (_Float32)UINT32_MAX) * scaler;

            iter += 2;

        }

        if(data->N_stars % 2 == 1) {
            raw_data.input = xoshiro_next(Xos);
            table[i][iter] = ((_Float32)raw_data.output[0] / (_Float32)UINT32_MAX) * scaler;
        }

        if(i == 2)
        {
            scaler = MAX_START_V;
        }
        else if(i == 5)
        {
            scaler = MAX_START_MASS;
        }

    }

    destroy_xoshiro(Xos);

}
