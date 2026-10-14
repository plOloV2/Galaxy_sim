set(CMAKE_C_COMPILER clang)

# Clang NVIDIA flags (Adjust -march=sm_86 to match target GPU architecture)
set(OMP_OFFLOAD_NVIDIA_COMPILE -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target=nvptx64-nvidia-cuda -march=sm_86)
set(OMP_OFFLOAD_NVIDIA_LINK -fopenmp-targets=nvptx64-nvidia-cuda)

# Clang AMD flags (Adjust -mcpu=gfx90a to match target GPU architecture)
set(OMP_OFFLOAD_AMD_COMPILE -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa -mcpu=gfx90a)
set(OMP_OFFLOAD_AMD_LINK -fopenmp-targets=amdgcn-amd-amdhsa)
