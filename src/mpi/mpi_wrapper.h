#ifndef __MPI_WRAPPER_H_
#define __MPI_WRAPPER_H_

#include <cstdio>
#include <cstring>
#include <mpi.h>
#include "macros.h"

namespace heat_transfer {

enum CHANNEL {
  LEFT, TOP, RIGHT, BOTTOM
};

enum DIRECTION {
  IN, OUT
};

enum TAG {
  LEFT_SEND = 10, UP_SEND, RIGHT_SEND, DOWN_SEND,
  RIGHT_RECV = LEFT_SEND, DOWN_RECV, LEFT_RECV, UP_RECV
};

class MPIWrapper {
 public:
  MPIWrapper() {}

  int Init() {
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz_);

    neighbors_[LEFT] = MPI_PROC_NULL;
    neighbors_[TOP] = MPI_PROC_NULL;
    neighbors_[RIGHT] = MPI_PROC_NULL;
    neighbors_[BOTTOM] = MPI_PROC_NULL;

    return 0;
  }

  int Destroy() {
    MPI_Finalize();
    return 0;
  }

  int CreateTopology(int height, int width) {
    int d[2] = {0, 0};
    MPI_Dims_create(comm_sz_, 2, d);

    // Check whether we can equally distribute the grid to the workers
    if (height % d[0] || width % d[1]) {
      if (!rank_) {
        std::fprintf(stderr,
            "Incompatible values for height, width and workers\n");
        std::fprintf(stderr,
            "Tried to split a %dx%d grid into a %dx%d topology\n",
            height, width, d[0], d[1]);
      }
      MPI_Barrier(MPI_COMM_WORLD);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Save the topology dimensions internally
    topology_height_ = d[0];
    topology_width_ = d[1];

    // Create topology
    const int periods[2] = {0, 0};  // No wrap
    MPI_Cart_create(
        MPI_COMM_WORLD,  // Input communicator
        2,               // 2D topology
        d,               // Topology dimensions
        periods,         // No wrap
        1,               // Allow reordering
        &topology_comm_);

    // Determine worker's (possibly new) rank and topology coordinates
    MPI_Comm_rank(topology_comm_, &rank_);
    MPI_Cart_coords(topology_comm_, rank_, 2, d);
    topology_coord_x_ = d[0];
    topology_coord_y_ = d[1];
    // Save block dimensions
    block_height_ = height / topology_height_;
    block_width_ = width / topology_width_;

    // Assign neighbors according to topology
    AssignNeighbors();

    // Create necessary types for column transfer
    CreateTypes();

    return 0;
  }

  int Send(const double *buf, CHANNEL ch, int tag) {
    int count;
    MPI_Datatype datatype;
    // Only send if there is a neighbor out there
    if (this->HasNeighbor(ch)) {
      if (ch == LEFT || ch == RIGHT) {
        count = 1;
        datatype = column_t_;
      }
      if (ch == TOP || ch == BOTTOM) {
        count = block_width_;
        datatype = MPI_DOUBLE;
      }
      MPI_Isend(
          buf,                   // outgoing buffer
          count,                 // how many elements?
          datatype,              // type of elements
          neighbors_[ch],        // destination
          tag,                   // tag
          topology_comm_,        // topology communicator
          &requests_[ch][OUT]);  // outgoing request
    }
    return 0;
  }

  int Receive(double *buf, CHANNEL ch, int tag) {
    int count;
    MPI_Datatype datatype;
    // Only receive if there is a neighbor out there
    if (this->HasNeighbor(ch)) {
      if (ch == LEFT || ch == RIGHT) {
        count = 1;
        datatype = column_t_;
      }
      if (ch == TOP || ch == BOTTOM) {
        count = block_width_;
        datatype = MPI_DOUBLE;
      }
      MPI_Irecv(
          buf,                  // incoming buffer
          count,                // how many elements?
          datatype,             // type of elements
          neighbors_[ch],       // source
          tag,                  // tag
          topology_comm_,       // topology communicator
          &requests_[ch][IN]);  // incoming request
    }
    return 0;
  }

  int Wait(CHANNEL ch) {
    if (this->HasNeighbor(ch)) {
      MPI_Wait(&requests_[ch][OUT], &status_[ch][OUT]);
      MPI_Wait(&requests_[ch][IN], &status_[ch][IN]);
    }
    return 0;
  }

  int ReduceTime(const double *local_time, double *global_time) const {
    return MPI_Reduce(
        local_time,   // send buffer
        global_time,  // recv buffer
        1,            // count
        MPI_DOUBLE,   // datatype (double)
        MPI_MAX,      // operator
        0,            // root
        topology_comm_);
  }

  int ReduceConvergenceCheck(const int *local_flag, int *global_flag) const {
    return MPI_Allreduce(
        local_flag,   // send buffer
        global_flag,  // recv buffer
        1,             // count
        MPI_INT,      // datatype (int-bool)
        MPI_MIN,       // operator
        topology_comm_);
  }

  /*
   * Only use after creating topology via MPIWrapper::CreateTopology
   */
  int Barrier() const {
    return MPI_Barrier(topology_comm_);
  }

  int PrintRoot(FILE *fp, const char *msg) const {
    if (rank_ == 0) std::fprintf(fp, msg);
    return 0;
  }

  int topology_height() const {
    return topology_height_;
  }

  int topology_width() const {
    return topology_width_;
  }

  int rank() const {
    return rank_;
  }

  int topology_coord_x() const {
    return topology_coord_x_;
  }

  int topology_coord_y() const {
    return topology_coord_y_;
  }

  int block_height() const {
    return block_height_;
  }

  int block_width() const {
    return block_width_;
  }

  bool HasNeighbor(CHANNEL ch) const {
    return neighbors_[ch] != MPI_PROC_NULL;
  }

 private:
  int AssignNeighbors() {
    // Assign upper and lower neighbors
    MPI_Cart_shift(
        topology_comm_,
        0,
        1,
        neighbors_ + TOP,
        neighbors_ + BOTTOM);
    // Assign left and right neighbors
    MPI_Cart_shift(
        topology_comm_,
        1,
        1,
        neighbors_ + LEFT,
        neighbors_ + RIGHT);
    return 0;
  }

  int CreateTypes() {
    MPI_Type_vector(block_height_, 1, block_width_ + 2, MPI_DOUBLE, &column_t_);
    MPI_Type_commit(&column_t_);
    return 0;
  }

  int rank_;  // Current process rank
  int comm_sz_;  // Communicator size

  int topology_height_;  // Cartesian topology height
  int topology_width_;  // Cartesian topology width
  int topology_comm_;  // Cartesian topology communicator

  int topology_coord_x_;  // Worker's topology X coordinate
  int topology_coord_y_;  // Worker's topology Y coordinate
  int block_height_;  // Worker's block height
  int block_width_;  // Worker's block width

  int neighbors_[4];  // Worker's neighbors
  MPI_Request requests_[4][2];  // Worker requests
  MPI_Status status_[4][2];  // Worker statuses

  MPI_Datatype column_t_;  // MPI datatype to *send* columns LEFT/RIGHT

  DISALLOW_COPY_AND_ASSIGN(MPIWrapper);
};

}  // namespace heat_transfer

#endif  // __MPI_WRAPPER_H_
