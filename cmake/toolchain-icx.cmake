set(CMAKE_C_COMPILER icx)

# Include OpenMP GPU & Hybrid sources
set(OMP_GPU_SRCS
    src/OpenMP_GPU/gravity_vector_b.c
)

# Preprocessor definitions for C code
set(OMP_GPU_DEFINITIONS
    OPENMP_GPU
    OPEN_MP_VENDOR="INTEL"
)

# ICX Intel GPU SPIR-V offload flags
set(OMP_OFFLOAD_COMPILE_FLAGS -fiopenmp -fopenmp-targets=spir64)
set(OMP_OFFLOAD_LINK_FLAGS    -fiopenmp -fopenmp-targets=spir64)

