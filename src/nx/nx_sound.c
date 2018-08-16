 
#include "osdepend.h"
#include "osd_cpu.h"
#include "driver.h"
 
#include <switch/audio/audio.h>
#include <switch/services/audout.h>

#define R_FAILED(res) ((res)<0)

static float delta_samples;
int samples_per_frame = 0;
short *samples_buffer;
short *conversion_buffer;
int usestereo = 1;



#define SAMPLERATE 48000
#define CHANNELCOUNT 2
#define FRAMERATE (1000 / 60)
#define SAMPLECOUNT SAMPLERATE / FRAMERATE
#define BYTESPERSAMPLE sizeof(uint16_t)
 
typedef struct nxAudioData
{
    void *buffer[2];
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
      (void)data;
      return (audio_data_size() + 0xfff) & ~0xfff;
}
 
nxAudioData *audioPtr = NULL;
 
int osd_start_audio_stream(int stereo)
{
	delta_samples = 0.0f;
	usestereo = stereo ? 1 : 0;
 
	/* determine the number of samples per frame */
	samples_per_frame = Machine->sample_rate / Machine->drv->frames_per_second;
 
	if (Machine->sample_rate == 0) return 0;
	
	audoutInitialize();	
	audoutStartAudioOut();
	
	audioPtr = (struct nxAudioData *)malloc(sizeof(nxAudioData));
	
	for (int i = 0; i < 2; i++) {
         
        audioPtr->buffer[i] = memalign(0x1000, audio_buffer_size(NULL));
        memset(audioPtr->buffer[i], 0, audio_buffer_size(NULL));
        audioPtr->source_buffer[i].next = NULL;
        audioPtr->source_buffer[i].buffer = audioPtr->buffer[i];
        audioPtr->source_buffer[i].buffer_size = audio_buffer_size(NULL);
        audioPtr->source_buffer[i].data_size = audio_data_size();
        audioPtr->source_buffer[i].data_offset = 0;
        audoutAppendAudioOutBuffer(&audioPtr->source_buffer[i]);
    }	
	
	audioPtr->released_buffer = NULL;
	
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
													&audioPtr->released_count, U64_MAX)))
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
 