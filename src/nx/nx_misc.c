 
 
#include "osdepend.h"
#include "osd_cpu.h"

#include "nx_mame.h"
 

#include "mame.h"
#include "cpuexec.h"
#include "palette.h"
#include "common.h"


#include <stdio.h>
#include <stdarg.h>



//---------------------------------------------------------------------
//  osd_malloc_debug
//---------------------------------------------------------------------
void *osd_malloc_debug( size_t size, const char *filename, unsigned int line, const char *function )
{
 
  void *ret = malloc( size );
 
  return ret;
}


//---------------------------------------------------------------------
//  osd_calloc_debug
//---------------------------------------------------------------------
void *osd_calloc_debug( size_t num, size_t size, const char *filename, unsigned int line, const char *function )
{
 

  void *ret = calloc( num, size );
 
  return ret;
}

//---------------------------------------------------------------------
//  osd_realloc_debug
//---------------------------------------------------------------------
void *osd_realloc_debug( void *memblock, size_t size, const char *filename, unsigned int line, const char *function )
{
  // [EBA] - "Safe" malloc, exits the program if the malloc fails, rather than
  // relying on MAME to actually check for failure (which it does not, in numerous
  // places)

  void *ret = realloc( memblock, size );

  return ret;
}

//---------------------------------------------------------------------
//  osd_malloc_retail
//---------------------------------------------------------------------
void *osd_malloc_retail( size_t size )
{
  // [EBA] - "Safe" malloc, exits the program if the malloc fails, rather than
  // relying on MAME to actually check for failure (which it does not, in numerous
  // places)

  void *ret = malloc( size );
 
  return ret;
}


//---------------------------------------------------------------------
//  osd_calloc_retail
//---------------------------------------------------------------------
void *osd_calloc_retail( size_t num, size_t size )
{
 
  void *ret = calloc( num, size );

  return ret;
}

//---------------------------------------------------------------------
//  osd_realloc_retail
//---------------------------------------------------------------------
void *osd_realloc_retail( void *memblock, size_t size )
{
  // [EBA] - "Safe" malloc, exits the program if the malloc fails, rather than
  // relying on MAME to actually check for failure (which it does not, in numerous
  // places)

  void *ret = realloc( memblock, size );

  return ret;
}


//---------------------------------------------------------------------
//	osd_display_loading_rom_message
//---------------------------------------------------------------------
int osd_display_loading_rom_message( const char *name, struct rom_load_data *romdata )
{
 
	
	RenderProgress(name,romdata);
	
	return 0;
}

//---------------------------------------------------------------------
//	osd_pause
//---------------------------------------------------------------------
void osd_pause( int paused )
{
}

//---------------------------------------------------------------------
//	logerror
//---------------------------------------------------------------------
void logerror( const char *fmt, ... )
{
 
  char buf[1024] = {0};

  va_list arg;
  va_start( arg, fmt );
  vsnprintf( buf, 1023, fmt, arg );
  va_end( arg );

	//PRINTMSG(( T_ERROR, buf ));
 

//debugload("error = %s\n",buf);
}

//---------------------------------------------------------------------
//	osd_print_error
//---------------------------------------------------------------------
void osd_print_error( const char *fmt, ... )
{  
  char buf[1024] = {0};

  va_list arg;
  va_start( arg, fmt );
  vsnprintf( buf, 1023, fmt, arg );
  va_end( arg );
 
   
  RenderMessage(buf);
 
}

//---------------------------------------------------------------------
//	osd_autobootsavestate
//---------------------------------------------------------------------
void osd_autobootsavestate( const char *gameName )
{
 
}

//---------------------------------------------------------------------
//	osd_die
//---------------------------------------------------------------------
void osd_die( const char *fmt, ... )
{
  char buf[1024] = {0};

  va_list arg;
  va_start( arg, fmt );
  vsnprintf( buf, 1023, fmt, arg );
  va_end( arg );
  
  //svcOutputDebugString(buf,1023);
 
}

double atan2(double x, double y)
{
  return 0.0f;
}