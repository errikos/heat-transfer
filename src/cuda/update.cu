#include <lcutil.h>
#include <timestamp.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 16  // The conservative approach


__global__ void updateKernel(unsigned int height, unsigned int width,
                             double *wgrid, double *ogrid) {
  // Determine coordinates of block within the data array and launch updates
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  if (!(i < height) || i < 0 || !(j < width) || j < 0) return;

  double old_val = *(ogrid + i*width + j);
  double left = j ? *(ogrid + i*width + (j-1)) : 0.0f;
  double top = i ? *(ogrid + (i-1)*width + j) : 0.0f;
  double right = (j != width-1) ? *(ogrid + i*width + (j+1)) : 0.0f;
  double bottom = (i != height-1) ? *(ogrid + (i+1)*width + j) : 0.0f;

  *(wgrid + i*width + j) = old_val
                           + 0.1 * (top + bottom - 2.0 * old_val)
                           + 0.1 * (right + left - 2.0 * old_val);
}


extern "C"
int updateGPU(double *host_array, unsigned int height, unsigned int width,
              unsigned int steps, float *elapsed_time) {

  unsigned int block_size = height * width;
  double *grids[2];

  // Allocate space for the the two grids in the GPU
  for (unsigned int i = 0; i != 2; ++i)
    CUDA_SAFE_CALL(cudaMalloc(&grids[i], block_size * sizeof(double)));

  // grids[0] will hold the initial data
  CUDA_SAFE_CALL(cudaMemcpy(grids[0], host_array, block_size * sizeof(double),
                            cudaMemcpyHostToDevice));
  // grids[1] initialized to zeroes
  CUDA_SAFE_CALL(cudaMemset(grids[1], 0, block_size * sizeof(double)));

  // Determine grid dimensions according to input array
  // x = ceil(height / BLOCK_SIZE)
  // y = ceil(width / BLOCK_SIZE)
  dim3 blocks(FRACTION_CEILING(height, BLOCK_SIZE),
              FRACTION_CEILING(width, BLOCK_SIZE));
  // BLOCK_SIZE x BLOCK_SIZE threads per block
  dim3 threads(BLOCK_SIZE, BLOCK_SIZE);

  int wgrid = 1;

  timestamp t_start;
  t_start = getTimestamp();  // Start timer

  for (unsigned int i = 0; i != steps; ++i) {
    // Fire update in kernel
    updateKernel<<<blocks, threads>>>(height, width,
                                      grids[wgrid], grids[1-wgrid]);
    // Wait for threads to reach this point
    CUDA_SAFE_CALL(cudaThreadSynchronize());
    wgrid = 1 - wgrid;
  }

  *elapsed_time = getElapsedTime(t_start) / 1000.0f;  // End timer

  // Get results back
  CUDA_SAFE_CALL(cudaMemcpy(host_array, grids[1-wgrid],
                            block_size * sizeof(double),
                            cudaMemcpyDeviceToHost));

  // Free space allocated in the GPU
  for (unsigned int i = 0; i != 2; ++i)
    CUDA_SAFE_CALL(cudaFree(grids[i]));

  return 0;
}
