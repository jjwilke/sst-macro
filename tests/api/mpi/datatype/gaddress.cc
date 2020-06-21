/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include <sstmac/replacements/mpi/mpi.h>
#include <stdio.h>
#include "mpitest.h"

namespace gaddress {
/**
static char MTEST_Descrip[] = "";
*/

int gaddress( int argc, char *argv[] )
{
    int errs = 0;
    int buf[10];
    MPI_Aint a1, a2;

    MTest_Init( &argc, &argv );
    
    MPI_Get_address( &buf[0], &a1 );
    MPI_Get_address( &buf[1], &a2 );
    
    if ((int)(a2-a1) != sizeof(int)) {
	errs++;
	printf( "Get address of two address did not return values the correct distance apart\n" );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}

}