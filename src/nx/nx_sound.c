 
#include "osdepend.h"
#include "osd_cpu.h"
#include "driver.h"
 
static float delta_samples;
int samples_per_frame = 0;
short *samples_buffer;
short *conversion_buffer;
int usestereo = 1;
 
 
int osd_start_audio_stream(int stereo)
{
	delta_samples = 0.0f;
	usestereo = stereo ? 1 : 0;

	Machine->sample_rate = 22050;
	
	/* determine the number of samples per frame */
	samples_per_frame = Machine->sample_rate / Machine->drv->frames_per_second;
	

	if (Machine->sample_rate == 0) return 0;

	samples_buffer = (short *) calloc(samples_per_frame, 2 + usestereo * 2);
	if (!usestereo) conversion_buffer = (short *) calloc(samples_per_frame, 4);
	
	
	return samples_per_frame;


}

int osd_update_audio_stream(INT16 *buffer)
{
		char sample_rate[100];
	sprintf(sample_rate,"sample_rate = %d\n",Machine->sample_rate);
	//svcOutputDebugString(sample_rate,100);
	
	memcpy(samples_buffer, buffer, samples_per_frame * (usestereo ? 4 : 2));
   	delta_samples += (Machine->sample_rate / Machine->drv->frames_per_second) - samples_per_frame;
	if (delta_samples >= 1.0f)
	{
		int integer_delta = (int)delta_samples;
		samples_per_frame += integer_delta;
		delta_samples -= integer_delta;
	}

	return samples_per_frame;
}



void osd_stop_audio_stream(void)
{
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
 