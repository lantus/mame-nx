 
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "osdepend.h"
#include "osd_cpu.h"

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
  BUTTON_ZR,
  BUTTON_LA_STICK,
  BUTTON_RA_STICK,
};

 
#ifdef __cplusplus
}
#endif


