 

#include "osd_cpu.h"
#include "osdepend.h"

 

//-------------------------------------------------------------
//	osd_cycles
//-------------------------------------------------------------
cycles_t osd_cycles( void )
{
	return svcGetSystemTick();
}	


//-------------------------------------------------------------
//	osd_cycles_per_second
//-------------------------------------------------------------
cycles_t osd_cycles_per_second( void )
{
	return 19200000;
}

//-------------------------------------------------------------
//	osd_profiling_ticks
//-------------------------------------------------------------
cycles_t osd_profiling_ticks( void )
{
	return svcGetSystemTick();
}



