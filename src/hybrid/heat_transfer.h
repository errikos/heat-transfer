#ifndef __HEAT_TRANSFER_H_
#define __HEAT_TRANSFER_H_

#include "heat_map.h"
#include "macros.h"
#include "mpi_wrapper.h"

namespace heat_transfer {

class HeatTransfer {
  public:
    HeatTransfer() {
    }

    int Init(int height, int width, int steps) {
        steps_ = steps;
        mpi_wrapper_.Init();

        // Create cartesian topology
        mpi_wrapper_.CreateTopology(height, width);

        // Initialize heat map for worker
        heat_map_.Init(mpi_wrapper_.block_height(), mpi_wrapper_.block_width(),
                       &mpi_wrapper_);
        return 0;
    }

    int Destroy() {
        mpi_wrapper_.Destroy();
        heat_map_.Destroy();
        return 0;
    }

    /*
     * HeatTransfer::Run - executes the heat transfer simulation
     */
    int Run() {
        double mpi_time_start, mpi_time_end, local_time, global_time;
        int converged_local = 0, converged_global = 0;
        int convergence_check = std::sqrt(steps_);

        // Wait until all workers reach this point
        mpi_wrapper_.Barrier();

        // Start timer
        mpi_time_start = MPI_Wtime();

        // Main simulation loop
        for (int i = 0; i != steps_; ++i) {
            // If convergence has been reached, then there is no reason to go on
            if (converged_global) {
                mpi_wrapper_.PrintRoot(
                    stdout, "Convergence was reached after %d iterations!\n",
                    i);
                break;
            }
            // Send and Receive messages (non-blocking)
            heat_map_.ExchangeMessages();
            // Update values of internal cells
            heat_map_.StandaloneUpdate();
            // Wait for incoming messages
            heat_map_.WaitForMessages();
            // Update values of edge cells
            heat_map_.CollaborativeUpdate();

            if (!(i % convergence_check)) {
                // Check whether convergence has been reached
                heat_map_.CheckConvergence(&converged_local);
            }

            // Reduce convergence flags and decide
            mpi_wrapper_.ReduceConvergenceCheck(&converged_local,
                                                &converged_global);
            // Change grids
            heat_map_.ExchangeGrids();
        }

        // Stop timer
        mpi_time_end = MPI_Wtime();

        // Calculate execution time by reducing local times and taking their max
        local_time = mpi_time_end - mpi_time_start;
        std::fprintf(stderr, "worker%d@%s, time: %.2f\n", mpi_wrapper_.rank(),
                     mpi_wrapper_.processor_name(), local_time);
        mpi_wrapper_.ReduceTime(&local_time, &global_time);

        mpi_wrapper_.PrintRoot(stdout, "\nElapsed time: %.2f sec\n",
                               global_time);

        return 0;
    }

  private:
    int steps_; // The maximum number of simulation steps

    HeatMap heat_map_;
    MPIWrapper mpi_wrapper_;

    DISALLOW_COPY_AND_ASSIGN(HeatTransfer);
};

} // namespace heat_transfer

#endif // __HEAT_TRANSFER_H_
