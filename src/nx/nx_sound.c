 
#include "osdepend.h"
#include "osd_cpu.h"
#include "driver.h"
 
 
INT32 osd_start_audio_stream( INT32 stereo )
{
	return 0;
}

//---------------------------------------------------------------------
//	osd_stop_audio_stream
//---------------------------------------------------------------------
void osd_stop_audio_stream( void )
{
	// if nothing to do, don't do it
	if( !Machine->sample_rate )
		return;
 
}

//---------------------------------------------------------------------
//	osd_update_audio_stream
//---------------------------------------------------------------------
INT32 osd_update_audio_stream( INT16 *buffer )
{
  
	return 0;
}

//---------------------------------------------------------------------
//	osd_set_mastervolume
//---------------------------------------------------------------------
void osd_set_mastervolume( INT32 attenuation )
{
  
}

//---------------------------------------------------------------------
//	osd_get_mastervolume
//---------------------------------------------------------------------
INT32 osd_get_mastervolume( void )
{
	return 0;
}

//---------------------------------------------------------------------
//	osd_sound_enable
//---------------------------------------------------------------------
void osd_sound_enable( INT32 enable )
{
 
}
 