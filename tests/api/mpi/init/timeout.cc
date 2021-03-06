/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include <sstmac/replacements/mpi.h>

namespace timeout {
/** This is a program that tests the ability of mpiexec to timeout a process
   after no more than 3 minutes.  By default, it will run for 5 minutes */
int timeout( int argc, char *argv[] )
{
    double t1;
    double deltaTime = 300;

    MPI_Init( 0, 0 );
    t1 = MPI_Wtime();
    while (MPI_Wtime() - t1 < deltaTime) ;
    MPI_Finalize( );
    return 0;
}

}