 
#include <stdio.h>

#include "osd_cpu.h"
#include "osdepend.h"
#include "osd_cpu.h"
#include "osdepend.h"
#include "inptport.h"
 
#define ANALOG_AS_DIGITAL_DEADZONE	8000
 
static struct JoystickInfo		g_joystickInfo[128] = {0,0,0};
static UINT32                   g_calibrationStep = 0;
static UINT32                   g_calibrationJoynum = 0;
 
UINT32							g_numOSDInputKeywords;


typedef struct JoystickPosition
{
    int32_t dx;
    int32_t dy;
} JoystickPosition;

JoystickPosition pos_left, pos_right;
 
typedef enum
{
    JOYSTICK_LEFT  = 0,
    JOYSTICK_RIGHT = 1,
    JOYSTICK_NUM_STICKS = 2,
} HidControllerJoystick;

typedef enum  	HidControllerID { 
  CONTROLLER_PLAYER_1 = 0, 
  CONTROLLER_PLAYER_2 = 1, 
  CONTROLLER_PLAYER_3 = 2, 
  CONTROLLER_PLAYER_4 = 3, 
  CONTROLLER_PLAYER_5 = 4, 
  CONTROLLER_PLAYER_6 = 5, 
  CONTROLLER_PLAYER_7 = 6, 
  CONTROLLER_PLAYER_8 = 7, 
  CONTROLLER_HANDHELD = 8, 
  CONTROLLER_UNKNOWN = 9, 
  CONTROLLER_P1_AUTO = 10 
} HidControllerID;

static HidControllerID pad_id[8] = {
    CONTROLLER_P1_AUTO, CONTROLLER_PLAYER_2,
    CONTROLLER_PLAYER_3, CONTROLLER_PLAYER_4,
    CONTROLLER_PLAYER_5, CONTROLLER_PLAYER_6,
    CONTROLLER_PLAYER_7, CONTROLLER_PLAYER_8
};

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
 
typedef enum  	HidControllerKeys { 
  KEY_A = (1U<<( 0 )), 
  KEY_B = (1U<<( 1 )), 
  KEY_X = (1U<<( 2 )), 
  KEY_Y = (1U<<( 3 )), 
  KEY_LSTICK = (1U<<( 4 )), 
  KEY_RSTICK = (1U<<( 5 )), 
  KEY_L = (1U<<( 6 )), 
  KEY_R = (1U<<( 7 )), 
  KEY_ZL = (1U<<( 8 )), 
  KEY_ZR = (1U<<( 9 )), 
  KEY_PLUS = (1U<<( 10 )), 
  KEY_MINUS = (1U<<( 11 )), 
  KEY_DLEFT = (1U<<( 12 )), 
  KEY_DUP = (1U<<( 13 )), 
  KEY_DRIGHT = (1U<<( 14 )), 
  KEY_DDOWN = (1U<<( 15 )), 
  KEY_LSTICK_LEFT = (1U<<( 16 )), 
  KEY_LSTICK_UP = (1U<<( 17 )), 
  KEY_LSTICK_RIGHT = (1U<<( 18 )), 
  KEY_LSTICK_DOWN = (1U<<( 19 )), 
  KEY_RSTICK_LEFT = (1U<<( 20 )), 
  KEY_RSTICK_UP = (1U<<( 21 )), 
  KEY_RSTICK_RIGHT = (1U<<( 22 )), 
  KEY_RSTICK_DOWN = (1U<<( 23 )), 
  KEY_SL = (1U<<( 24 )), 
  KEY_SR = (1U<<( 25 )), 
  KEY_TOUCH = (1U<<( 26 )), 
  KEY_JOYCON_RIGHT = (1U<<( 0 )), 
  KEY_JOYCON_DOWN = (1U<<( 1 )), 
  KEY_JOYCON_UP = (1U<<( 2 )), 
  KEY_JOYCON_LEFT = (1U<<( 3 )), 
  KEY_UP = KEY_DUP | KEY_LSTICK_UP | KEY_RSTICK_UP, 
  KEY_DOWN = KEY_DDOWN | KEY_LSTICK_DOWN | KEY_RSTICK_DOWN, 
  KEY_LEFT = KEY_DLEFT | KEY_LSTICK_LEFT | KEY_RSTICK_LEFT, 
  KEY_RIGHT = KEY_DRIGHT | KEY_LSTICK_RIGHT | KEY_RSTICK_RIGHT 
} HidControllerKeys;
	 
typedef enum nxJoyType {
	JT_LSTICK_UP = 0,
	JT_LSTICK_DOWN,
	JT_LSTICK_LEFT,
	JT_LSTICK_RIGHT,
	JT_RSTICK_UP,
	JT_RSTICK_DOWN,        // 5
	JT_RSTICK_LEFT,
	JT_RSTICK_RIGHT,
	JT_DPAD_UP,
	JT_DPAD_DOWN,
	JT_DPAD_LEFT,          // 10
	JT_DPAD_RIGHT,
	JT_BUTTON
} nxJoyType;

typedef enum ButtonID {
  BUTTON_A = 0,
  BUTTON_X,
  BUTTON_B,
  BUTTON_Y,
  BUTTON_LEFT_TRIGGER,
  BUTTON_RIGHT_TRIGGER,
  BUTTON_PLUS,
  BUTTON_MINUS,   
  BUTTON_ZL,               
  BUTTON_ZR                
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
    ADDENTRY( "A",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_A ),              STDCODE( BUTTON1 ) );
    ADDENTRY( "X",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_X ),              STDCODE( BUTTON2 ) );
    ADDENTRY( "B",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_B ),              STDCODE( BUTTON3 ) );
    ADDENTRY( "Y",            JOYCODE( stickIndex, JT_BUTTON, BUTTON_Y ),              STDCODE( BUTTON4 ) );
    ADDENTRY( "LTrig",        JOYCODE( stickIndex, JT_BUTTON, BUTTON_LEFT_TRIGGER ),   STDCODE( BUTTON5 ) );
    ADDENTRY( "RTrig",        JOYCODE( stickIndex, JT_BUTTON, BUTTON_RIGHT_TRIGGER ),  STDCODE( BUTTON6 ) );
    ADDENTRY( "Plus",         JOYCODE( stickIndex, JT_BUTTON, BUTTON_PLUS ),           STDCODE( START ) );
    ADDENTRY( "Minus",        JOYCODE( stickIndex, JT_BUTTON, BUTTON_MINUS ),          STDCODE( SELECT ) );     
    ADDENTRY( "LThumb",       JOYCODE( stickIndex, JT_BUTTON, BUTTON_ZL ),             CODE_OTHER );
    ADDENTRY( "RThumb",       JOYCODE( stickIndex, JT_BUTTON, BUTTON_ZR ),             CODE_OTHER );


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
				case BUTTON_PLUS:
					return (buttons & KEY_PLUS);
				case BUTTON_MINUS:
					return (buttons & KEY_MINUS);				 
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
  