#include <cstdlib>
#include <iostream>

#include "argparse.h"
#include "heat_transfer.h"
using namespace std;
using namespace heat_transfer;

int main(int argc, char *argv[]) {
    // Setup and parse arguments
    ArgumentParser parser("mpi_heat", "HeatTransfer implementation in MPI");
    parser.AddArgument("-h", "Grid height", true);
    parser.AddArgument("-w", "Grid width", true);
    parser.AddArgument("-s", "Time steps", true);
    if (parser.Parse(argc, argv))
        exit(EXIT_FAILURE);

    int height = parser.GetValue<int>("-h");
    int width = parser.GetValue<int>("-w");
    int steps = parser.GetValue<int>("-s");

    // Setup and run simulation with given arguments
    HeatTransfer simulation;
    simulation.Init(height, width, steps);
    simulation.Run();

    // Bye, bye...
    simulation.Destroy();
    exit(EXIT_SUCCESS);
}
