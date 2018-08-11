 
#include <stdio.h>

#include "osd_cpu.h"
#include "osdepend.h"
 
#define RETRO_DEVICE_ID_JOYPAD_B        0
#define RETRO_DEVICE_ID_JOYPAD_Y        1
#define RETRO_DEVICE_ID_JOYPAD_SELECT   2
#define RETRO_DEVICE_ID_JOYPAD_START    3
#define RETRO_DEVICE_ID_JOYPAD_UP       4
#define RETRO_DEVICE_ID_JOYPAD_DOWN     5
#define RETRO_DEVICE_ID_JOYPAD_LEFT     6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT    7
#define RETRO_DEVICE_ID_JOYPAD_A        8
#define RETRO_DEVICE_ID_JOYPAD_X        9
#define RETRO_DEVICE_ID_JOYPAD_L       10
#define RETRO_DEVICE_ID_JOYPAD_R       11
#define RETRO_DEVICE_ID_JOYPAD_L2      12
#define RETRO_DEVICE_ID_JOYPAD_R2      13
#define RETRO_DEVICE_ID_JOYPAD_L3      14
#define RETRO_DEVICE_ID_JOYPAD_R3      15 
 
#define EMIT_NX_PAD(INDEX) \
    {"RetroPad" #INDEX " Left", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_LEFT, JOYCODE_##INDEX##_LEFT}, \
    {"RetroPad" #INDEX " Right", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_RIGHT, JOYCODE_##INDEX##_RIGHT}, \
    {"RetroPad" #INDEX " Up", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_UP, JOYCODE_##INDEX##_UP}, \
    {"RetroPad" #INDEX " Down", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_DOWN, JOYCODE_##INDEX##_DOWN}, \
    {"RetroPad" #INDEX " B", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_B, JOYCODE_##INDEX##_BUTTON1}, \
    {"RetroPad" #INDEX " Y", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_Y, JOYCODE_##INDEX##_BUTTON2}, \
    {"RetroPad" #INDEX " X", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_X, JOYCODE_##INDEX##_BUTTON3}, \
    {"RetroPad" #INDEX " A", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_A, JOYCODE_##INDEX##_BUTTON4}, \
    {"RetroPad" #INDEX " L", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_L, JOYCODE_##INDEX##_BUTTON5}, \
    {"RetroPad" #INDEX " R", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_R, JOYCODE_##INDEX##_BUTTON6}, \
    {"RetroPad" #INDEX " L2", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_L2, JOYCODE_##INDEX##_BUTTON7}, \
    {"RetroPad" #INDEX " R2", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_R2, JOYCODE_##INDEX##_BUTTON8}, \
    {"RetroPad" #INDEX " L3", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_L3, JOYCODE_##INDEX##_BUTTON9}, \
    {"RetroPad" #INDEX " R3", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_R3, JOYCODE_##INDEX##_BUTTON10}, \
    {"RetroPad" #INDEX " Start", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_START, JOYCODE_##INDEX##_START}, \
    {"RetroPad" #INDEX " Select", ((INDEX - 1) * 18) + RETRO_DEVICE_ID_JOYPAD_SELECT, JOYCODE_##INDEX##_SELECT}

struct JoystickInfo jsItems[] =
{
    EMIT_NX_PAD(1),
    EMIT_NX_PAD(2),
    {0, 0, 0}
};

 

//---------------------------------------------------------------------
//	osd_get_joy_list
//---------------------------------------------------------------------
const struct JoystickInfo *osd_get_joy_list( void )
{
  
	return jsItems;
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
 
 


