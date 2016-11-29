#ifndef __LCUTIL_H_
#define __LCUTIL_H_

#include <cuda.h>
#include <stdio.h>

#define CUDA_SAFE_CALL(call)                                                   \
    {                                                                          \
        cudaError err = call;                                                  \
        if (cudaSuccess != err) {                                              \
            fprintf(stderr, "Cuda error in file '%s' in line %i : %s.\n",      \
                    __FILE__, __LINE__, cudaGetErrorString(err));              \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    \
}

#define FRACTION_CEILING(numerator, denominator)                               \
    ((numerator + denominator - 1) / denominator)

#endif // __LCUTIL_H_
