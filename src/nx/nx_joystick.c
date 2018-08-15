 
#include <stdio.h>

#include "osd_cpu.h"
#include "osdepend.h"
#include "osd_cpu.h"
#include "osdepend.h"
#include "inptport.h"
 
static struct JoystickInfo		g_joystickInfo[128] = {0,0,0};
static UINT32                   g_calibrationStep = 0;
static UINT32                   g_calibrationJoynum = 0;
 
UINT32							g_numOSDInputKeywords;


#define JOYNAME( _string__ )  				sprintf( name, "J%d %s", stickIndex + 1, _string__ )
#define BEGINENTRYMAP()                  	UINT32 joycount = 0
#define ADDENTRY( _name__, _code__, _standardCode__ )    JOYNAME( _name__ ); nxAddEntry( name, (_code__), (_standardCode__), &joycount )
#define STDCODE( cde )    					(stickIndex == 0 ? JOYCODE_1_##cde : (stickIndex == 1 ? JOYCODE_2_##cde : (stickIndex == 2 ? JOYCODE_3_##cde : JOYCODE_4_##cde )))
#define JOYCODE(joy, type, index)	      	((index) | ((type) << 8) | ((joy) << 12))
#define JOYINDEX(joycode)			        ((joycode) & 0xff)
#define JT(joycode)			                (((joycode) >> 8) & 0xf)
#define JOYNUM(joycode)				        (((joycode) >> 12) & 0xf)
#define AXISCODE( _stick__, _axis__ )   	joyoscode_to_code( JOYCODE( _stick__, _axis__, 0 ) )
#define BUTTONCODE( _stick__, _ID__ )   	joyoscode_to_code( JOYCODE( _stick__, JT_BUTTON, _ID__ ) )
 
	 
typedef enum NXJoyType {
	KEY_A = 0, KEY_B, KEY_X, KEY_Y,
    KEY_LSTICK, KEY_RSTICK,
    KEY_L, KEY_R,
    KEY_ZL, KEY_ZR,
    KEY_PLUS, KEY_MINUS,
    KEY_DLEFT, KEY_DUP, KEY_DRIGHT, KEY_DDOWN,
    KEY_LSTICK_LEFT, KEY_LSTICK_UP, KEY_LSTICK_RIGHT, KEY_LSTICK_DOWN,
    KEY_RSTICK_LEFT, KEY_RSTICK_UP, KEY_RSTICK_RIGHT, KEY_RSTICK_DOWN, JT_BUTTON
} NXJoyType;
 
void nxInitializeJoystick( void )
{
	BEGINENTRYMAP();
	INT32 stickIndex = 0;

	for( ; stickIndex < 1; ++stickIndex )
	{
    char name[32];
 
      // DPad
    ADDENTRY( "DPAD UP",      JOYCODE( stickIndex, KEY_DUP, 0 ),        STDCODE( UP ) );
    ADDENTRY( "DPAD RIGHT",   JOYCODE( stickIndex, KEY_DRIGHT , 0 ),    STDCODE( RIGHT ) );
    ADDENTRY( "DPAD DOWN",    JOYCODE( stickIndex, KEY_DDOWN, 0 ),      STDCODE( DOWN ) );
    ADDENTRY( "DPAD LEFT",    JOYCODE( stickIndex, KEY_DLEFT , 0 ),     STDCODE( LEFT ) );

      // Left analog
    AXISCODE( stickIndex, KEY_LSTICK_UP );
    AXISCODE( stickIndex, KEY_LSTICK_RIGHT );
    AXISCODE( stickIndex, KEY_LSTICK_DOWN );
    AXISCODE( stickIndex, KEY_LSTICK_LEFT );
    ADDENTRY( "LA UP",        JOYCODE( stickIndex, KEY_LSTICK_UP, 0 ),     CODE_OTHER );
    ADDENTRY( "LA RIGHT",     JOYCODE( stickIndex, KEY_LSTICK_RIGHT , 0 ), CODE_OTHER );
    ADDENTRY( "LA DOWN",      JOYCODE( stickIndex, KEY_LSTICK_DOWN, 0 ),   CODE_OTHER );
    ADDENTRY( "LA LEFT",      JOYCODE( stickIndex, KEY_LSTICK_LEFT , 0 ),  CODE_OTHER );

      // Right analog
    AXISCODE( stickIndex, KEY_RSTICK_UP );
    AXISCODE( stickIndex, KEY_RSTICK_RIGHT );
    AXISCODE( stickIndex, KEY_RSTICK_DOWN );
    AXISCODE( stickIndex, KEY_RSTICK_LEFT );
    ADDENTRY( "RA UP",        JOYCODE( stickIndex, KEY_RSTICK_UP, 0 ),     CODE_OTHER );
    ADDENTRY( "RA RIGHT",     JOYCODE( stickIndex, KEY_RSTICK_RIGHT , 0 ), CODE_OTHER );
    ADDENTRY( "RA DOWN",      JOYCODE( stickIndex, KEY_RSTICK_DOWN, 0 ),   CODE_OTHER );
    ADDENTRY( "RA LEFT",      JOYCODE( stickIndex, KEY_RSTICK_LEFT , 0 ),  CODE_OTHER );

      // Buttons
 
    ADDENTRY( "A",            JOYCODE( stickIndex, 8, KEY_A ),              STDCODE( BUTTON1 ) );
    ADDENTRY( "X",            JOYCODE( stickIndex, 8, KEY_X ),              STDCODE( BUTTON2 ) );
    ADDENTRY( "B",            JOYCODE( stickIndex, 8, KEY_B ),              STDCODE( BUTTON3 ) );
    ADDENTRY( "Y",            JOYCODE( stickIndex, 8, KEY_Y ),              STDCODE( BUTTON4 ) );
    ADDENTRY( "LTrig",        JOYCODE( stickIndex, 8, KEY_L ),   			STDCODE( BUTTON5 ) );
    ADDENTRY( "RTrig",        JOYCODE( stickIndex, 8, KEY_R ),  			STDCODE( BUTTON6 ) );
    ADDENTRY( "Plus",         JOYCODE( stickIndex, 8, KEY_PLUS ),          	STDCODE( START ) );
    ADDENTRY( "Minus",        JOYCODE( stickIndex, 8, KEY_MINUS ),       	STDCODE( SELECT ) );

  }
}
 
 
//---------------------------------------------------------------------
//	osd_get_joy_list
//---------------------------------------------------------------------
const struct JoystickInfo *osd_get_joy_list( void )
{  
	 
	
	
	return g_joystickInfo;
}

//---------------------------------------------------------------------
//	osd_is_joy_pressed
//---------------------------------------------------------------------
int osd_is_joy_pressed( int joycode )
{
	int32_t joyindex = JOYINDEX(joycode);
	int32_t joytype = JT(joycode);
	int32_t joynum = JOYNUM(joycode);

	hidScanInput();
	uint64_t buttons = hidKeysHeld(10);
	
	 
	switch( joytype )
	{
		case 8: 
			return (buttons & joyindex);		 
			break;
/*
		case KEY_DUP:
			return (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
		case KEY_DOWN:
			return (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
		case KEY_LEFT:
			return (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
		case KEY_RIGHT:
			return (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);*/

	 

	}
	
	 
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
 
 
void nxAddEntry( const char *name, INT32 code, INT32 standardCode, UINT32 *joycount )
{
	struct JoystickInfo *ji = NULL;
	struct ik *inputkeywords;
  
	ji = &g_joystickInfo[*joycount];

	ji->name = strdup( name );
	if( !ji->name )
	{	 
		osd_print_error( "Out of memory!" );
		return;
	}

    // Convert spaces in ji->name to '_'
	{
		char *cur = ji->name;
		while( *cur )
		{
			if( *cur == ' ' )
			*cur = '_';
		++cur;
		}
	}

	ji->code = code;
	ji->standardcode = standardCode;

	++(*joycount);
}
  