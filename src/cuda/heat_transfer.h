#ifndef __HEAT_TRANSFER_H_
#define __HEAT_TRANSFER_H_

#include "macros.h"
#include <cstdio>
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <lcutil.h>

namespace heat_transfer {

extern "C" int updateGPU(double *host_array, unsigned int height,
                         unsigned int width, unsigned int steps_,
                         float *elapsed_time);

class HeatTransfer {
  public:
    HeatTransfer() : grid_(NULL) {
    }

    int Init(unsigned int height, unsigned int width, unsigned int steps) {
        steps_ = steps;
        height_ = height;
        width_ = width;
        InitGrid();
        return 0;
    }

    int Destroy() {
        if (grid_ != NULL)
            delete[] grid_;
        return 0;
    }

    int Run() {
        float elapsed_time = 0.0;
        updateGPU(grid_, height_, width_, steps_, &elapsed_time);
        std::fprintf(stderr, "\nElapsed time: %.2f sec\n", elapsed_time);
        return 0;
    }

  private:
    int GetCellValue(unsigned int i, unsigned int j, double *val) const {
        // Out of bounds
        if (!(i < height_) || i < 0 || !(j < width_) || j < 0)
            return 1;
        *val = *(grid_ + i * width_ + j); // *val = grid_[i][j]
        return 0;
    }

    int SetCellValue(unsigned int i, unsigned int j, double val) const {
        // Out of bounds
        if (!(i < height_) || i < 0 || !(j < width_) || j < 0)
            return 1;
        *(grid_ + i * width_ + j) = val; // grid_[i][j] = val
        return 0;
    }

    int InitGrid() {
        unsigned int block_size = height_ * width_;
        grid_ = new double[block_size];
        // Initialize grid values
        for (unsigned int i = 0; i != height_; ++i)
            for (unsigned int j = 0; j != width_; ++j) {
                double val = (i + 1) * (height_ - i) * (j + 1) * (width_ - j);
                SetCellValue(i, j, val);
            }
        return 0;
    }

    void PrintGrid() const {
        double val;
        for (unsigned int i = 0; i != height_; ++i) {
            for (unsigned int j = 0; j != width_; ++j) {
                GetCellValue(i, j, &val);
                std::printf("  %7.2f", val);
            }
            std::printf("\n");
        }
        std::printf("\n");
    }

    unsigned int steps_; // The maximum number of simulation steps

    unsigned int height_;
    unsigned int width_;
    double *grid_;

    DISALLOW_COPY_AND_ASSIGN(HeatTransfer);
};

} // namespace heat_transfer

#endif // __HEAT_TRANSFER_H_
