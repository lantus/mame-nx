#include <stdio.h>
#include "osd_cpu.h"
#include "osdepend.h"
 
#include "nx_joystick.h"
 
  //! Macros for redefining input sequences
#define REMAP_SEQ_20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t) { InputSeq newSeq = SEQ_DEF_20((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m),(n),(o),(p),(q),(r),(s),(t)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_19(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)   { InputSeq newSeq = SEQ_DEF_19((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m),(n),(o),(p),(q),(r),(s)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)     { InputSeq newSeq = SEQ_DEF_18((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m),(n),(o),(p),(q),(r)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)       { InputSeq newSeq = SEQ_DEF_17((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m),(n),(o),(p),(q)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)         { InputSeq newSeq = SEQ_DEF_16((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m),(n),(o),(p)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)           { InputSeq newSeq = SEQ_DEF_15((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m),(n),(o)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)             { InputSeq newSeq = SEQ_DEF_14((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m),(n)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_13(a,b,c,d,e,f,g,h,i,j,k,l,m)               { InputSeq newSeq = SEQ_DEF_13((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l),(m)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_12(a,b,c,d,e,f,g,h,i,j,k,l)                 { InputSeq newSeq = SEQ_DEF_12((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k),(l)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_11(a,b,c,d,e,f,g,h,i,j,k)                   { InputSeq newSeq = SEQ_DEF_11((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),(k)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_10(a,b,c,d,e,f,g,h,i,j)                     { InputSeq newSeq = SEQ_DEF_10((a),(b),(c),(d),(e),(f),(g),(h),(i),(j)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_9(a,b,c,d,e,f,g,h,i)                        { InputSeq newSeq = SEQ_DEF_9((a),(b),(c),(d),(e),(f),(g),(h),(i)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_8(a,b,c,d,e,f,g,h)                          { InputSeq newSeq = SEQ_DEF_8((a),(b),(c),(d),(e),(f),(g),(h)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_7(a,b,c,d,e,f,g)                            { InputSeq newSeq = SEQ_DEF_7((a),(b),(c),(d),(e),(f),(g)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_6(a,b,c,d,e,f)                              { InputSeq newSeq = SEQ_DEF_6((a),(b),(c),(d),(e),(f)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_5(a,b,c,d,e)                                { InputSeq newSeq = SEQ_DEF_5((a),(b),(c),(d),(e)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_4(a,b,c,d)                                  { InputSeq newSeq = SEQ_DEF_4((a),(b),(c),(d)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_3(a,b,c)                                    { InputSeq newSeq = SEQ_DEF_3((a),(b),(c)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_2(a,b)                                      { InputSeq newSeq = SEQ_DEF_2((a),(b)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_1(a)                                        { InputSeq newSeq = SEQ_DEF_1((a)); memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
#define REMAP_SEQ_0()				                          { InputSeq newSeq = SEQ_DEF_0; memcpy( entry->seq, newSeq, sizeof(entry->seq) ); }
 
static int g_systemInitialized = 0;
 
const struct KeyboardInfo nxKeys[] =
{
	{0, 0, 0}
};
 
//---------------------------------------------------------------------
//	osd_customize_inputport_defaults
//---------------------------------------------------------------------
void osd_customize_inputport_defaults( struct ipd *defaults )
{

  if( g_systemInitialized == 0)
  {
    nxInitializeJoystick();
    g_systemInitialized = 1;
  }	 
  
  nxCustomizeInputPortDefaults( defaults );
}

 
int osd_is_key_pressed( int keycode )
{
	return 0;
}

int osd_readkey_unicode( int flush )
{
 
	return 0;
}

//---------------------------------------------------------------------
//	osd_get_key_list
//---------------------------------------------------------------------
const struct KeyboardInfo *osd_get_key_list( void )
{
	return nxKeys;
}

void nxCustomizeInputPortDefaults( struct ipd *defaults )
{

	UINT32 i = 0;

	for( ; defaults[i].type != IPT_END; ++i )
	{
		struct ipd *entry = &defaults[i];

		switch( entry->type )
		{
		case IPT_UI_CONFIGURE:
			REMAP_SEQ_7(  	BUTTONCODE( 0, BUTTON_RA_STICK ), CODE_OR, 
							BUTTONCODE( 1, BUTTON_RA_STICK ), CODE_OR,
							BUTTONCODE( 2, BUTTON_RA_STICK ), CODE_OR,
							BUTTONCODE( 3, BUTTON_RA_STICK ));
			break;
		case IPT_UI_CANCEL:
			REMAP_SEQ_11(	JOYCODE_1_SELECT, JOYCODE_1_START, CODE_OR, 
							JOYCODE_2_SELECT, JOYCODE_2_START, CODE_OR,
							JOYCODE_3_SELECT, JOYCODE_3_START, CODE_OR,
							JOYCODE_4_SELECT, JOYCODE_4_START);		
			break;
		case IPT_UI_ON_SCREEN_DISPLAY:
			/*REMAP_SEQ_7(  	BUTTONCODE( 0, BUTTON_LA ), CODE_OR, 
							BUTTONCODE( 1, BUTTON_LA ), CODE_OR,
							BUTTONCODE( 2, BUTTON_LA ), CODE_OR,
							BUTTONCODE( 3, BUTTON_LA ));*/
		 
			break;


		case IPT_UI_UP:
		  REMAP_SEQ_15( JOYCODE_1_UP, CODE_OR, AXISCODE( 0, JT_LSTICK_UP ), CODE_OR,
						JOYCODE_2_UP, CODE_OR, AXISCODE( 1, JT_LSTICK_UP ), CODE_OR,
						JOYCODE_3_UP, CODE_OR, AXISCODE( 2, JT_LSTICK_UP ), CODE_OR,
						JOYCODE_4_UP, CODE_OR, AXISCODE( 3, JT_LSTICK_UP ));
		  break;

				// *** IPT_UI_LEFT *** //
		case IPT_UI_LEFT:
		  REMAP_SEQ_15( JOYCODE_1_LEFT, CODE_OR, AXISCODE( 0, JT_LSTICK_LEFT ), CODE_OR,
						JOYCODE_2_LEFT, CODE_OR, AXISCODE( 1, JT_LSTICK_LEFT ), CODE_OR,
						JOYCODE_3_LEFT, CODE_OR, AXISCODE( 2, JT_LSTICK_LEFT ), CODE_OR,
						JOYCODE_4_LEFT, CODE_OR, AXISCODE( 3, JT_LSTICK_LEFT ) );
		  break;

				// *** IPT_UI_DOWN *** //
		case IPT_UI_DOWN:
		  REMAP_SEQ_15( JOYCODE_1_DOWN, CODE_OR, AXISCODE( 0, JT_LSTICK_DOWN ), CODE_OR,
						JOYCODE_2_DOWN, CODE_OR, AXISCODE( 1, JT_LSTICK_DOWN ), CODE_OR,
						JOYCODE_3_DOWN, CODE_OR, AXISCODE( 2, JT_LSTICK_DOWN ), CODE_OR,
						JOYCODE_4_DOWN, CODE_OR, AXISCODE( 3, JT_LSTICK_DOWN ) );
		  break;

				// *** IPT_UI_RIGHT *** //
		case IPT_UI_RIGHT:
		  REMAP_SEQ_15( JOYCODE_1_RIGHT, CODE_OR, AXISCODE( 0, JT_LSTICK_RIGHT ), CODE_OR,
						JOYCODE_2_RIGHT, CODE_OR, AXISCODE( 1, JT_LSTICK_RIGHT ), CODE_OR,
						JOYCODE_3_RIGHT, CODE_OR, AXISCODE( 2, JT_LSTICK_RIGHT ), CODE_OR,
						JOYCODE_4_RIGHT, CODE_OR, AXISCODE( 3, JT_LSTICK_RIGHT ) );
		break;
		}
	}
}