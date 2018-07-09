 
#include <stdio.h>

#include "osd_cpu.h"
#include "osdepend.h"
 

//---------------------------------------------------------------------
//	osd_get_joy_list
//---------------------------------------------------------------------
const struct JoystickInfo *osd_get_joy_list( void )
{
  //	return a list of all available joystick inputs (see input.h)
	return NULL;
}

//---------------------------------------------------------------------
//	osd_is_joy_pressed
//---------------------------------------------------------------------
int osd_is_joy_pressed( int joycode )
{
 
	
	return 0;
}


//---------------------------------------------------------------------
//	osd_is_joystick_axis_code
//---------------------------------------------------------------------
int osd_is_joystick_axis_code( int joycode )
{
 
	return 0;
}

//---------------------------------------------------------------------
//	osd_joystick_needs_calibration
//---------------------------------------------------------------------
int osd_joystick_needs_calibration( void )
{
 
  return 0;
}


//---------------------------------------------------------------------
//	osd_joystick_start_calibration
//---------------------------------------------------------------------
void osd_joystick_start_calibration( void )
{
 
}

//---------------------------------------------------------------------
//	osd_joystick_calibrate_next
//---------------------------------------------------------------------
const char *osd_joystick_calibrate_next( void )
{
 
	return NULL;
}

//---------------------------------------------------------------------
//	osd_joystick_calibrate
//---------------------------------------------------------------------
void osd_joystick_calibrate( void )
{
 
}

//---------------------------------------------------------------------
//	osd_joystick_end_calibration
//---------------------------------------------------------------------
void osd_joystick_end_calibration( void )
{
/* Postprocessing (e.g. saving joystick data to config) */
}

//---------------------------------------------------------------------
//	osd_lightgun_read
//---------------------------------------------------------------------
void osd_lightgun_read(int player, int *deltax, int *deltay)
{
 
}

//---------------------------------------------------------------------
//	osd_trak_read
//---------------------------------------------------------------------
void osd_trak_read(int player, int *deltax, int *deltay)
{
 
 
}


//---------------------------------------------------------------------
//	osd_analogjoy_read
//---------------------------------------------------------------------
void osd_analogjoy_read(	int player,
													int analog_axis[MAX_ANALOG_AXES], 
													InputCode analogjoy_input[MAX_ANALOG_AXES] )
{
 
}
 
 


