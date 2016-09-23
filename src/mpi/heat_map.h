#ifndef __HEAT_MAP_H_
#define __HEAT_MAP_H_

#include <cmath>
#include <vector>
#include "mpi_wrapper.h"

namespace heat_transfer {

class HeatMap {
 public:
   HeatMap() : working_grid_(0), block_height_(0), block_width_(0) {
    for (int i = 0; i != 2; ++i)
      grids_[i] = NULL;
    mpi_wrapper_ = NULL;
  }

  /*
   * Init: Initializes the heat map (grid).
   */
  int Init(int block_height, int block_width, MPIWrapper *mpi_wrapper) {
    block_height_ = block_height;
    block_width_ = block_width;
    mpi_wrapper_ = mpi_wrapper;

    // Allocate space for the heat map, plus the incoming message buffers
    // and initialize to zeroes
    unsigned int block_size = (block_height_ + 2) * (block_width_ + 2);
    for (unsigned int g = 0; g != 2; ++g) {
      grids_[g] = new double[block_size];
      for (unsigned int i = 0; i != block_size; ++i)
        grids_[g][i] = 0.0;
    }

    // Initialize block
    // Calculate total size and offsets
    unsigned int x = mpi_wrapper_->topology_height() * block_height_;
    unsigned int y = mpi_wrapper_->topology_width() * block_width_;
    unsigned int off_x = mpi_wrapper_->topology_coord_x() * block_height_;
    unsigned int off_y = mpi_wrapper_->topology_coord_y() * block_width_;
    // Fill initial values
    for (unsigned int i = 1; i != 1+block_height_; ++i)
      for (unsigned int j = 1; j != 1+block_width_; ++j) {
        double val = (i+off_x) * (x - (i-1+off_x))
                      * (j+off_y) * (y - (j-1+off_y));
        SetCellValue(i, j, 0, val);
      }

    return 0;
  }

  int Destroy() {
    for (int i = 0; i != 2; ++i)
      if (grids_[i] != NULL) delete[] grids_[i];
    return 0;
  }

  int ExchangeMessages() {
    double *addr;
    // Send/Receive LEFT
    addr = grids_[working_grid_] + (block_width_+2) + 1;
    mpi_wrapper_->Send(addr, LEFT, LEFT_SEND);
    addr = grids_[working_grid_] + (block_width_+2);
    mpi_wrapper_->Receive(addr, LEFT, LEFT_RECV);
    // Send/Receive TOP
    addr = grids_[working_grid_] + (block_width_+2) + 1;
    mpi_wrapper_->Send(addr, TOP, UP_SEND);
    addr = grids_[working_grid_] + 1;
    mpi_wrapper_->Receive(addr, TOP, UP_RECV);
    // Send/Receive RIGHT
    addr = grids_[working_grid_] + 2 * (block_width_+2) - 2;
    mpi_wrapper_->Send(addr, RIGHT, RIGHT_SEND);
    addr = grids_[working_grid_] + 2 * (block_width_+2) - 1;
    mpi_wrapper_->Receive(addr, RIGHT, RIGHT_RECV);
    // Send/Receive BOTTOM
    addr = grids_[working_grid_] + (block_height_ * (block_width_+2)) + 1;
    mpi_wrapper_->Send(addr, BOTTOM, DOWN_SEND);
    addr = grids_[working_grid_] + ((block_height_+1) * (block_width_+2)) + 1;
    mpi_wrapper_->Receive(addr, BOTTOM, DOWN_RECV);
    return 0;
  }

  int CellUpdate(unsigned int i, unsigned int j) {
    double val[4];  // Four values, LEFT, TOP, RIGHT, BOTTOM cell
    double old_val = 0.0, new_val = 0.0;
    int wg = working_grid_, g = 1 - working_grid_;

    GetCellValue(i, j, wg, &old_val);
    GetCellValue(i, j-1, wg, &val[LEFT]);
    GetCellValue(i-1, j, wg, &val[TOP]);
    GetCellValue(i, j+1, wg, &val[RIGHT]);
    GetCellValue(i+1, j, wg, &val[BOTTOM]);
    new_val = old_val
              + 0.1 * (val[TOP] + val[BOTTOM] - 2.0 * old_val)
              + 0.1 * (val[RIGHT] + val[LEFT] - 2.0 * old_val);
    SetCellValue(i, j, g, new_val);

    return 0;
  }

  int StandaloneUpdate() {
    for (unsigned int i = 2; i != block_height_; ++i)
      for (unsigned int j = 2; j != block_width_; ++j)
        CellUpdate(i, j);
    return 0;
  }

  int WaitForMessages() {
    // Wait for LEFT neighbor send/recv statuses
    mpi_wrapper_->Wait(LEFT);
    // Wait for TOP neighbor send/recv statuses
    mpi_wrapper_->Wait(TOP);
    // Wait for RIGHT neighbor send/recv statuses
    mpi_wrapper_->Wait(RIGHT);
    // Wait for BOTTOM neighbor send/recv statuses
    mpi_wrapper_->Wait(BOTTOM);
    return 0;
  }

  int CollaborativeUpdate() {
    // Update top and bottom rows
    for (unsigned int j = 1; j != 1+block_width_; ++j) {
      CellUpdate(1, j);
      CellUpdate(block_height_, j);
    }
    // Update left and right columns
    for (unsigned int i = 1; i != 1+block_height_; ++i) {
      CellUpdate(i, 1);
      CellUpdate(i, block_width_);
    }
    return 0;
  }

  int CheckConvergence(int *converged) const {
    double val1, val2;
    *converged = 0;
    for (unsigned int i = 1; i != 1+block_height_; ++i)
      for (unsigned int j = 1; j != 1+block_width_; ++j) {
        GetCellValue(i, j, working_grid_, &val1);
        GetCellValue(i, j, 1 - working_grid_, &val2);
        if (std::fabs(val1 - val2) < 0.001f) {
          *converged = 1;
          return 0;
        }
      }
    return 0;
  }

  void ExchangeGrids() {
    working_grid_ = 1 - working_grid_;
  }

  void PrintGrid(int rank) const {
    if (mpi_wrapper_->rank() == rank) {
      double val;
      for (int g = 0; g != 2; ++g) {
        std::printf("=== Ext. Block %d of worker: %d\n", g, rank);
        for (unsigned int i = 0; i != block_height_+2; ++i) {
          for (unsigned int j = 0; j != block_width_+2; ++j) {
            GetCellValue(i, j, g, &val);
            std::printf("  %.2f", val);
          }
          std::printf("\n");
        }
        std::printf("\n");
      }
    }
  }

 private:
  int GetCellValue(unsigned int i, unsigned int j, int grid, double *val) const {
    // Out of bounds
    if (!(i < block_height_+2) || i < 0 || !(j < block_width_+2) || j < 0)
      return 1;
    // Invalid grid specifier
    if (grid < 0 || grid > 1)
      return 1;
    double *gridp = grids_[grid];
    *val = *(gridp + i*(block_width_+2) + j);  // *val = grid[i][j]
    return 0;
  }

  int SetCellValue(unsigned int i, unsigned int j, int grid, double val) const {
    // Out of bounds
    if (!(i < block_height_+2) || i < 0 || !(j < block_width_+2) || j < 0)
      return 1;
    // Invalid grid specifier
    if (grid < 0 || grid > 1)
      return 1;
    double *gridp = grids_[grid];
    *(gridp + i*(block_width_+2) + j) = val;  // grid[i][j] = val
    return 0;
  }

  // Two maps (ie. grids),
  double *grids_[2];  // which are used interchangeably at each time step
  int working_grid_;  // Working grid indicator

  unsigned int block_height_;
  unsigned int block_width_;

  MPIWrapper *mpi_wrapper_;
};

}  // namespace heat_transfer

#endif  // __HEAT_MAP_H_
