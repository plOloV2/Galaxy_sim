set(CMAKE_C_COMPILER icx)

# ICX Intel iGPU flags
set(OMP_OFFLOAD_INTEL_COMPILE -fiopenmp -fopenmp-targets=spir64)
set(OMP_OFFLOAD_INTEL_LINK -fiopenmp -fopenmp-targets=spir64)
