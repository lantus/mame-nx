 

#include "osd_cpu.h"
#include "osdepend.h"
#include <time.h>

 

//-------------------------------------------------------------
//	osd_cycles
//-------------------------------------------------------------
cycles_t osd_cycles( void )
{
	struct timeval t; 

    gettimeofday(&t, 0); 

 
    return (t.tv_sec * 1) + (t.tv_usec);
}	


//-------------------------------------------------------------
//	osd_cycles_per_second
//-------------------------------------------------------------
cycles_t osd_cycles_per_second( void )
{
	return 1;
}

//-------------------------------------------------------------
//	osd_profiling_ticks
//-------------------------------------------------------------
cycles_t osd_profiling_ticks( void )
{
	return osd_cycles();
}



