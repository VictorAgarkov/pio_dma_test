#ifndef _UAC2_H_INCLUDED
	#define _UAC2_H_INCLUDED
	
	#include "usbd_core.h"
	#include "usbd_audio.h"

	#define AUDIO_SPEAKER_FRAME_SIZE_BYTE ((UAC_BIT_RESOLUTION + 7) / 8)
	#define AUDIO_SPEAKER_RESOLUTION_BIT    UAC_BIT_RESOLUTION

	#define AUDIO_MIC_FRAME_SIZE_BYTE    ((UAC_BIT_RESOLUTION + 7) / 8)
	#define AUDIO_MIC_RESOLUTION_BIT       UAC_BIT_RESOLUTION
	
	/* AudioFreq * DataSize (2 bytes) * NumChannels */
	#define AUDIO_OUT_PACKET ((AUDIO_OUT_MAX_FREQ * (AUDIO_SPEAKER_FRAME_SIZE_BYTE * OUT_CHANNEL_NUM + 1)) / 1000)
	#define AUDIO_IN_PACKET  ((AUDIO_IN_MAX_FREQ  * (AUDIO_MIC_FRAME_SIZE_BYTE     *  IN_CHANNEL_NUM + 1)) / 1000)
	//#define AUDIO_OUT_PACKET 1020
	//#define AUDIO_IN_PACKET  1020

	#if (AUDIO_IN_PACKET > 1023)
		#warning "AUDIO_IN_PACKET owersized"
	#endif

	#if (AUDIO_OUT_PACKET > 1023)
		#warning AUDIO_OUT_PACKET owersized
	#endif

	#if ((AUDIO_IN_PACKET + AUDIO_OUT_PACKET) > 1480)
		#warning IN_PACKET + AUDIO_OUT_PACKET owersized
	#endif
	
	typedef enum
	{
		TX_STATE_IDLE,           // мик. остановлен
		TX_STATE_STARTED,        // мик. запущен (usbd_audio_open), но ничего не отправлено
		TX_STATE_CB_WAIT,        // отправлен пакет с аудиоданными, ждём callback
		TX_STATE_CB_COMPLETE     // callback получен, можно отправлять следующий пакет данных
	}
	TX_STATE_e;

	extern volatile TX_STATE_e g_USB_tx_state;
	
	extern USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t uac_read_buffer[AUDIO_OUT_PACKET];
	extern USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t uac_write_buffer[AUDIO_IN_PACKET];
	
	
	void audio_v2_init(uint8_t busid, uintptr_t reg_base);
	void audio_v2_test(uint8_t busid);
	void audio_send_mic_buff(uint8_t busid, int bytes_to_send);
	
#endif // _UAC2_H_INCLUDED