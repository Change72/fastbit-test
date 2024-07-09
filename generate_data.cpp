#include <iostream>
#include <cstdlib>
#include <vector>

#include <unistd.h>
#include <adios2.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int rank, size;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::string outputBPFile = "vector_1d.bp";

    adios2::ADIOS adios(MPI_COMM_WORLD);
    adios2::IO writer_io = adios.DeclareIO("WriterIO");

    adios2::Engine writer_engine = writer_io.Open(outputBPFile, adios2::Mode::Write); 

    size_t n = 100;

    adios2::Variable<double> var = writer_io.DefineVariable<double>("x", {size*n}, {rank*n}, {n});

    std::vector<double> vector_1d;

    for (size_t i = 0; i < n; i++)
    {
        vector_1d.push_back(double(i+rank*n));
    }
    
    writer_engine.BeginStep();
    writer_engine.Put(var, vector_1d.data());
    writer_engine.EndStep();


    writer_engine.Close();

    MPI_Finalize();

}