
 
#include "osdepend.h"
#include "osd_cpu.h"
#include "driver.h"

#include "nx_mame.h"
#include <switch.h>
  
int samples_per_frame = 0;
  
#define SAMPLERATE 48000
#define CHANNELCOUNT 2
#define FRAMERATE (1000 / 60)
#define SAMPLECOUNT SAMPLERATE / FRAMERATE
#define BYTESPERSAMPLE sizeof(uint16_t)
 
typedef struct nxAudioData
{     
    AudioOutBuffer source_buffer[2];
    AudioOutBuffer *released_buffer;
    uint32_t released_count;

} nxAudioData; 
 
static uint32_t audio_data_size()
{
      return (SAMPLECOUNT * CHANNELCOUNT * BYTESPERSAMPLE);
}

static size_t audio_buffer_size(void *data)
{      
      return (audio_data_size() + 0xfff) & ~0xfff;
}
 
nxAudioData *audioPtr = NULL;
 
int osd_start_audio_stream(int stereo)
{	 

	if (Machine->sample_rate == 0) return 0;
	
	/* determine the number of samples per frame */
	samples_per_frame = Machine->sample_rate / Machine->drv->frames_per_second;	
	
	audoutInitialize();	
	audoutStartAudioOut();
	
	if (!audioPtr)
	{
		audioPtr = (struct nxAudioData *)calloc(1,sizeof(*audioPtr));
	
		for (int i = 0; i < 2; i++) {
			  
			audioPtr->source_buffer[i].next = NULL;
			audioPtr->source_buffer[i].buffer = aligned_alloc(0x1000, audio_buffer_size(NULL));
			audioPtr->source_buffer[i].buffer_size = audio_buffer_size(NULL);
			audioPtr->source_buffer[i].data_size = audio_data_size();
			audioPtr->source_buffer[i].data_offset = 0;
			memset(audioPtr->source_buffer[i].buffer,0x00,audio_buffer_size(NULL));
			audoutAppendAudioOutBuffer(&audioPtr->source_buffer[i]);
		}	
	
		audioPtr->released_buffer = NULL;
	}
	
	return samples_per_frame;


}

int osd_update_audio_stream(INT16 *buffer)
{	 
	 
	int samplerate_buffer_size = (Machine->sample_rate / Machine->drv->frames_per_second);
	

	if (!audioPtr->released_buffer)
    {
		audoutGetReleasedAudioOutBuffer(&audioPtr->released_buffer, &audioPtr->released_count);
		
		if (audioPtr->released_count < 1)
		{
			audioPtr->released_buffer = NULL;
			
			while (audioPtr->released_buffer == NULL)
            {      
				audioPtr->released_count = 0;
				if (R_FAILED(audoutWaitPlayFinish(	&audioPtr->released_buffer,
													&audioPtr->released_count, UINT64_MAX)))
                {
                 
                }
			}
 
								 
		}
		 		
		audioPtr->released_buffer->data_size = 0;
								
	}
	
	
	if (samplerate_buffer_size > audio_buffer_size(NULL) - audioPtr->released_buffer->data_size)
        samplerate_buffer_size = audio_buffer_size(NULL) - audioPtr->released_buffer->data_size;
 
	memcpy((uint8_t *)(audioPtr->released_buffer->buffer) + audioPtr->released_buffer->data_size, buffer, samplerate_buffer_size * 4);
	audioPtr->released_buffer->data_size += samplerate_buffer_size*4;
	audioPtr->released_buffer->buffer_size = audio_buffer_size(NULL);
	
	if (audioPtr->released_buffer->data_size > (48000) / 1000)
    {	
		audoutAppendAudioOutBuffer(audioPtr->released_buffer);
		audioPtr->released_buffer = NULL;
	}
	return samplerate_buffer_size;
}



void osd_stop_audio_stream(void)
{
	 	
	if (audioPtr)
	{
		 
		audoutStopAudioOut();
		audoutExit();

		for (int i = 0; i < 2; i++)
			  free(audioPtr->source_buffer[i].buffer);

		free(audioPtr);
		audioPtr = NULL;
	}		
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
 