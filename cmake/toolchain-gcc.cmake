set(CMAKE_C_COMPILER                gcc)

# GCC NVIDIA flags
set(OMP_OFFLOAD_NVIDIA_COMPILE -foffload=nvptx-none -fno-math-errno)
set(OMP_OFFLOAD_NVIDIA_LINK -foffload=nvptx-none)

# GCC AMD flags
set(OMP_OFFLOAD_AMD_COMPILE -foffload=amdgcn-amdhsa -fno-math-errno)
set(OMP_OFFLOAD_AMD_LINK -foffload=amdgcn-amdhsa)
