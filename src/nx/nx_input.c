 
#include "osd_cpu.h"
#include "osdepend.h"
 
const struct KeyboardInfo nxKeys[] =
{
	{0, 0, 0}
};
 
//---------------------------------------------------------------------
//	osd_customize_inputport_defaults
//---------------------------------------------------------------------
void osd_customize_inputport_defaults( struct ipd *defaults )
{
	 
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