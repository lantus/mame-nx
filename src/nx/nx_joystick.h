 
#include <stdio.h>

#include "osd_cpu.h"
#include "osdepend.h"
#include "osd_cpu.h"
#include "osdepend.h"
#include "inptport.h"
#include "nx_joystick.h"

#include "nx_mame.h" 
#include <switch.h> 
 
#define ANALOG_AS_DIGITAL_DEADZONE	100
 
static struct JoystickInfo		g_joystickInfo[128] = {0,0,0};
static UINT32                   g_calibrationStep = 0;
static UINT32                   g_calibrationJoynum = 0;
 
UINT32							g_numOSDInputKeywords;


JoystickPosition pos_left, pos_right;
 
static HidControllerID pad_id[8] = {
    CONTROLLER_P1_AUTO, CONTROLLER_PLAYER_2,
    CONTROLLER_PLAYER_3, CONTROLLER_PLAYER_4,
    CONTROLLER_PLAYER_5, CONTROLLER_PLAYER_6,
    CONTROLLER_PLAYER_7, CONTROLLER_PLAYER_8
};
 
 
void nxInitializeJoystick( void )
{
	BEGINENTRYMAP();
	INT32 stickIndex = 0;

	for( ; stickIndex < 4; ++stickIndex )
	{
    char name[32];
 
      // DPad
    ADDENTRY( "DPAD UP",      JOYCODE( stickIndex, JT_DPAD_UP, 0 ),        STDCODE( UP ) );
    ADDENTRY( "DPAD RIGHT",   JOYCODE( stickIndex, JT_DPAD_RIGHT , 0 ),    STDCODE( RIGHT ) );
    ADDENTRY( "DPAD DOWN",    JOYCODE( stickIndex, JT_DPAD_DOWN, 0 ),      STDCODE( DOWN ) );
    ADDENTRY( "DPAD LEFT",    JOYCODE( stickIndex, JT_DPAD_LEFT , 0 ),     STDCODE( LEFT ) );

      // Left analog
    AXISCODE( stickIndex, JT_LSTICK_UP );
    AXISCODE( stickIndex, JT_LSTICK_RIGHT );
    AXISCODE( stickIndex, JT_LSTICK_DOWN );
    AXISCODE( stickIndex, JT_LSTICK_LEFT );
    ADDENTRY( "LA UP",        JOYCODE( stickIndex, JT_LSTICK_UP, 0 ),     CODE_OTHER );
    ADDENTRY( "LA RIGHT",     JOYCODE( stickIndex, JT_LSTICK_RIGHT , 0 ), CODE_OTHER );
    ADDENTRY( "LA DOWN",      JOYCODE( stickIndex, JT_LSTICK_DOWN, 0 ),   CODE_OTHER );
    ADDENTRY( "LA LEFT",      JOYCODE( stickIndex, JT_LSTICK_LEFT , 0 ),  CODE_OTHER );

      // Right analog
    AXISCODE( stickIndex, JT_RSTICK_UP );
    AXISCODE( stickIndex, JT_RSTICK_RIGHT );
    AXISCODE( stickIndex, JT_RSTICK_DOWN );
    AXISCODE( stickIndex, JT_RSTICK_LEFT );
    ADDENTRY( "RA UP",        JOYCODE( stickIndex, JT_RSTICK_UP, 0 ),     CODE_OTHER );
    ADDENTRY( "RA RIGHT",     JOYCODE( stickIndex, JT_RSTICK_RIGHT , 0 ), CODE_OTHER );
    ADDENTRY( "RA DOWN",      JOYCODE( stickIndex, JT_RSTICK_DOWN, 0 ),   CODE_OTHER );
    ADDENTRY( "RA LEFT",      JOYCODE( stickIndex, JT_RSTICK_LEFT , 0 ),  CODE_OTHER );

      // Buttons
 
    BUTTONCODE( stickIndex, BUTTON_ZL );
    BUTTONCODE( stickIndex, BUTTON_ZR );
    ADDENTRY( "A",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_A ),              STDCODE( BUTTON5 ) );
    ADDENTRY( "B",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_B ),              STDCODE( BUTTON4 ) );
    ADDENTRY( "X",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_X ),              STDCODE( BUTTON1 ) );
    ADDENTRY( "Y",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_Y ),              STDCODE( BUTTON2 ) );
    ADDENTRY( "LTrig",        JOYCODE( stickIndex, JT_BUTTON, BUTTON_LEFT_TRIGGER ),   STDCODE( BUTTON7 ) );
    ADDENTRY( "RTrig",        JOYCODE( stickIndex, JT_BUTTON, BUTTON_RIGHT_TRIGGER ),  STDCODE( BUTTON8 ) );
    ADDENTRY( "Plus",         JOYCODE( stickIndex, JT_BUTTON, BUTTON_PLUS ),           STDCODE( START ) );
    ADDENTRY( "Minus",        JOYCODE( stickIndex, JT_BUTTON, BUTTON_MINUS ),          STDCODE( SELECT ) ); 
	ADDENTRY( "ZL",           JOYCODE( stickIndex, JT_BUTTON, BUTTON_ZL ),             STDCODE( BUTTON6 ) ); 
	ADDENTRY( "ZR",        	  JOYCODE( stickIndex, JT_BUTTON, BUTTON_ZR ),             STDCODE( BUTTON3 ) );
	ADDENTRY( "RA Stick",     JOYCODE( stickIndex, JT_BUTTON, BUTTON_RA_STICK ),       CODE_OTHER );
	ADDENTRY( "LA Stick",     JOYCODE( stickIndex, JT_BUTTON, BUTTON_LA_STICK ),       CODE_OTHER );


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
	
	int buttons = hidKeysHeld(pad_id[joynum]);	
	hidJoystickRead(&pos_left,  pad_id[joynum], JOYSTICK_LEFT);
    hidJoystickRead(&pos_right, pad_id[joynum], JOYSTICK_RIGHT);	
 	 
	switch( joytype )	
	{
		case JT_BUTTON:
			switch( joyindex )
			{
				case BUTTON_A:
					return (buttons & KEY_A);
				case BUTTON_X:
					return (buttons & KEY_X);
				case BUTTON_B:
					return (buttons & KEY_B);
				case BUTTON_Y:
					return (buttons & KEY_Y);
				case BUTTON_LEFT_TRIGGER:
					return (buttons & KEY_L);
				case BUTTON_RIGHT_TRIGGER:
					return (buttons & KEY_R);
				case BUTTON_ZL:
					return (buttons & KEY_ZL);
				case BUTTON_ZR:
					return (buttons & KEY_ZR);
				case BUTTON_PLUS:
					return (buttons & KEY_PLUS);
				case BUTTON_MINUS:
					return (buttons & KEY_MINUS);		
				case BUTTON_LA_STICK:
					return (buttons & KEY_LSTICK);	
				case BUTTON_RA_STICK:
					return (buttons & KEY_RSTICK);						
			}
			break;

		case JT_DPAD_UP:
			return (buttons & KEY_DUP);
		case JT_DPAD_DOWN:
			return (buttons & KEY_DDOWN);
		case JT_DPAD_LEFT:
			return (buttons & KEY_DLEFT);
		case JT_DPAD_RIGHT:
			return (buttons & KEY_DRIGHT);
			
		case JT_LSTICK_UP:
			return (pos_left.dy > ANALOG_AS_DIGITAL_DEADZONE );
		case JT_LSTICK_DOWN:
			return (pos_left.dy < -ANALOG_AS_DIGITAL_DEADZONE );
		case JT_LSTICK_LEFT:
			return (pos_left.dx < -ANALOG_AS_DIGITAL_DEADZONE );
		case JT_LSTICK_RIGHT:
			return (pos_left.dx > ANALOG_AS_DIGITAL_DEADZONE );

		case JT_RSTICK_UP:
			return (pos_right.dy  > ANALOG_AS_DIGITAL_DEADZONE );
		case JT_RSTICK_DOWN:
			return (pos_right.dy  < -ANALOG_AS_DIGITAL_DEADZONE );
		case JT_RSTICK_LEFT:
			return (pos_right.dx  < -ANALOG_AS_DIGITAL_DEADZONE );
		case JT_RSTICK_RIGHT:
			return (pos_right.dx  > ANALOG_AS_DIGITAL_DEADZONE ); 

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
  
