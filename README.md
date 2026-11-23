# Galaxy_sim

Gravitational N-body simulation featuring a benchmarking suite for various computational backends (single- and multi-threaded CPU, SIMD, GPU CUDA, and OpenMP GPU offloading).

---

## Requirements

* **CMake** >= 4.1
* **C23 compiler** with OpenMP support (e.g., GCC)
* *(Optional)* **NVIDIA CUDA Toolkit** (`nvcc`) — required for the CUDA backend (`-DENABLE_CUDA=ON`)
* *(Optional)* **GCC NVPTX offload** (`gcc-offload-nvptx`) — required for the OpenMP GPU backend
* *(Optional)* **NVIDIA Nsight Systems** (`nsys`) — for performance profiling

> **Note:** The project compiles with `-Werror` enabled. When building with GCC, pass `-Wno-error=unused-result` in `CMAKE_C_FLAGS` to prevent `scanf` return value warnings from failing the build.

---

## Building the Project

The compiled `./Galaxy_sim` binary is placed directly in the repository root directory.

### 1. Debug Mode

In `Debug` mode (compiled with `-O2 -ggdb3`, without `-DNDEBUG`), test mode (`-t`) prompts for the number of benchmark runs and allows verifying numerical accuracy against the reference `basic_cpu` implementation.

#### Variant A: Debug with CUDA (`-DENABLE_CUDA=ON`)

Using CMake presets (with optional OpenMP GPU support):
```bash
cmake --preset Debug_GCC \
  -DENABLE_CUDA=ON \
  -DOMP_GPU_SRCS="src/OpenMP_GPU/gravity_vector_b.c" \
  -DOMP_GPU_DEFINITIONS='OPENMP_GPU;OPEN_MP_VENDOR="NVIDIA"' \
  -DCMAKE_C_FLAGS="-Wno-error=unused-result -foffload=nvptx-none"

cmake --build --preset "Debug GCC"
```

Without OpenMP GPU (CPU + CUDA only):
```bash
cmake --preset Debug_GCC \
  -DENABLE_CUDA=ON \
  -DCMAKE_C_FLAGS="-Wno-error=unused-result"

cmake --build --preset "Debug GCC"
```

#### Variant B: Debug without CUDA / CPU Only (`-DENABLE_CUDA=OFF`)

```bash
cmake --preset Debug_GCC \
  -DENABLE_CUDA=OFF \
  -DCMAKE_C_FLAGS="-Wno-error=unused-result"

cmake --build --preset "Debug GCC"
```

---

### 2. Release Mode

In `Release` mode, full compiler optimizations (`-O3`, `-fomit-frame-pointer`, LTO/IPO) and `-DNDEBUG` are enabled. This is the recommended configuration for performance benchmarking and profiling.

#### Variant A: Release with CUDA (`-DENABLE_CUDA=ON`)

Using the `Release_GCC` preset (with optional OpenMP GPU support):
```bash
cmake --preset Release_GCC \
  -DCMAKE_C_COMPILER=gcc-13 \
  -DENABLE_CUDA=ON \
  -DOMP_GPU_SRCS="src/OpenMP_GPU/gravity_vector_b.c" \
  -DOMP_GPU_DEFINITIONS='OPENMP_GPU;OPEN_MP_VENDOR="NVIDIA"' \
  -DCMAKE_C_FLAGS="-Wno-error=unused-result -foffload=nvptx-none"

cmake --build --preset "Release GCC"
```

Or without presets (using a standalone `build-release` directory):
```bash
cmake -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc-13 \
  -DENABLE_CUDA=ON \
  -DCMAKE_C_FLAGS="-Wno-error=unused-result"

cmake --build build-release -j
```

> **Note (LTO & CUDA host compiler):** Since `Release` enables IPO/LTO, the C compiler version must match the host compiler used by `nvcc` (e.g., `gcc-13` for CUDA 12.x). If your system defaults to a newer GCC (such as GCC 14 or 15) and linking fails with an `LTO version` mismatch error, explicitly pass `-DCMAKE_C_COMPILER=gcc-13` during CMake configuration.

#### Variant B: Release without CUDA / CPU Only (`-DENABLE_CUDA=OFF`)

Using the `Release_GCC` preset:
```bash
cmake --preset Release_GCC \
  -DENABLE_CUDA=OFF \
  -DCMAKE_C_FLAGS="-Wno-error=unused-result"

cmake --build --preset "Release GCC"
```

Or without presets (using a standalone `build-release` directory):
```bash
cmake -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_CUDA=OFF \
  -DCMAKE_C_FLAGS="-Wno-error=unused-result"

cmake --build build-release -j
```

---

## Running the Application

### Command-Line Options

| Flag | Description |
| :--- | :--- |
| `-t`, `--test <N1> [N2 ...]` | Runs automated benchmarks for the specified galaxy sizes (`N >= 2`). |
| `-s`, `--seed <number>` | Sets the PRNG seed (`number > 0`) for reproducible initial star data. |

---

### 1. Interactive Mode (Single Algorithm)

Run without the `-t` flag:
```bash
./Galaxy_sim
# Or with a custom seed:
./Galaxy_sim -s 42
```

You will be prompted to enter:
1. **Number of stars** (e.g., `10000`).
2. **Algorithm number:**
   * `1` — Single thread (basic)
   * `2` — Single thread (naive)
   * `3` — Single thread SIMD
   * `4` — Multi thread (basic)
   * `5` — Multi thread (naive)
   * `6` — Multi thread SIMD
   * `7` — GPU CUDA *(requires `-DENABLE_CUDA=ON`)*
   * `8` — GPU OpenMP *(requires `OPENMP_GPU` configuration)*

The selected algorithm runs in a continuous loop, printing the execution time of each step to the console. Press `Ctrl+C` to stop.

---

### 2. Test / Benchmark Mode (`-t`)

Sequentially benchmarks all compiled algorithms across the specified galaxy sizes:
```bash
# Single galaxy size (10,000 stars)
./Galaxy_sim -t 10000

# Multiple sizes with a fixed RNG seed
./Galaxy_sim -t 1000 10000 50000 -s 42
```

* **In `Debug` mode:** the CLI prompts for the number of test runs and whether to verify results against `basic_cpu` (`1` = Yes, `0` = No).
* **In `Release` mode:** the benchmark automatically runs `100` iterations per algorithm without result verification (`Check_Status = DISABLED`).

#### Generated Output Files
Upon completion, two CSV reports are written to the working directory:
* `benchmark_passes.csv` — per-step execution times (`Galaxy_Size,Algorithm,Pass,Time_s,Check_Status`).
* `benchmark_summary.csv` — mean execution time and sample standard deviation (`Galaxy_Size,Algorithm,Runs,Average_Time_s,Std_Dev_s`).

---

## Profiling with NVIDIA Nsight Systems (`nsys`)

For profiling, build the project in **`Release`** mode with **`-DENABLE_CUDA=ON`**.

### 1. Profiling the Full Benchmark (`-t`)

Because benchmark mode exits automatically upon completion, `nsys` captures the entire run and generates both the `.nsys-rep` report and the `.csv` files:
```bash
nsys profile --trace=cuda,nvtx --stats=true -o report_test ./Galaxy_sim -t 10000 -s 42
```

### 2. Profiling a Single Algorithm (Interactive Mode)

Since interactive mode runs in an infinite loop, stop the program with **`Ctrl+C`** after a few iterations (or limit capture time with `--duration`):
```bash
nsys profile --trace=cuda,nvtx --stats=true -o report_single ./Galaxy_sim -s 42
```
1. Enter the galaxy size and select an algorithm (e.g., `7` for CUDA).
2. Wait for 3–5 iterations to print in the console, then press **`Ctrl+C`**.
3. `nsys` will catch the interrupt, print summary statistics to the terminal, and save `report_single.nsys-rep`.

### 3. Viewing the Report in the GUI (Windows / WSL2)

1. Open **NVIDIA Nsight Systems** on Windows.
2. Go to **File -> Open** and select `report_test.nsys-rep` or `report_single.nsys-rep`.

> **WSL2 Tip (GPU Hardware Timeline):** If the `GPU` row is missing from the timeline (showing only CPU-side `CUDA API` calls), open **NVIDIA Control Panel** on Windows -> **Desktop -> Enable Developer Settings**, then navigate to **Manage GPU Performance Counters** and select **Allow access to the GPU performance counters to all users**.