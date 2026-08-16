/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "uac2.h"


#ifndef ARRAYSIZE
	#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif // ARRAYSIZE

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define USBD_MAX_POWER 100
#define USBD_LANGID_STRING 1033

#ifdef CONFIG_USB_HS
	#define EP_INTERVAL               0x04
	#define FEEDBACK_ENDP_PACKET_SIZE 0x04
#else
	#define EP_INTERVAL               0x01
	#define FEEDBACK_ENDP_PACKET_SIZE 0x04
#endif



#define AUDIO_OUT_EP 0x02
#define AUDIO_IN_EP 0x81
#define AUDIO_OUT_FEEDBACK_EP 0x83

#define AUDIO_OUT_CLOCK_ID 0x01
#define AUDIO_OUT_FU_ID 0x03
#define AUDIO_IN_CLOCK_ID 0x05
#define AUDIO_IN_FU_ID 0x07

#define BMCONTROL (AUDIO_V2_CONTROL_MUTE | AUDIO_V2_CONTROL_VOLUME)

#define AUDIO_FREQ_TO_FEEDBACK_10_14(freq)  (((freq) << 11) / 125)



#if IN_CHANNEL_NUM == 0
	#define INPUT_CTRL DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x00000000
#elif IN_CHANNEL_NUM == 1
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x00000001
#elif IN_CHANNEL_NUM == 2
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x00000003
#elif IN_CHANNEL_NUM == 3
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x00000007
#elif IN_CHANNEL_NUM == 4
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x0000000f
#elif IN_CHANNEL_NUM == 5
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x0000001f
#elif IN_CHANNEL_NUM == 6
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x0000003F
#elif IN_CHANNEL_NUM == 7
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x0000007f
#elif IN_CHANNEL_NUM == 8
	#define INPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define INPUT_CH_ENABLE 0x000000ff
#endif

#if OUT_CHANNEL_NUM == 0
	#define OUTPUT_CTRL DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x00000000
#elif OUT_CHANNEL_NUM == 1
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x00000001
#elif OUT_CHANNEL_NUM == 2
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x00000003
#elif OUT_CHANNEL_NUM == 3
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x00000007
#elif OUT_CHANNEL_NUM == 4
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x0000000f
#elif OUT_CHANNEL_NUM == 5
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x0000001f
#elif OUT_CHANNEL_NUM == 6
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x0000003F
#elif OUT_CHANNEL_NUM == 7
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x0000007f
#elif OUT_CHANNEL_NUM == 8
	#define OUTPUT_CTRL DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL), DBVAL(BMCONTROL)
	#define OUTPUT_CH_ENABLE 0x000000ff
#endif



#define CORE_DESCRIPTORS_SIZE (	0 +                         \
	AUDIO_V2_SIZEOF_AC_CLOCK_SOURCE_DESC +                  \
	AUDIO_V2_SIZEOF_AC_INPUT_TERMINAL_DESC +                \
	AUDIO_V2_SIZEOF_AC_FEATURE_UNIT_DESC(OUT_CHANNEL_NUM) + \
	AUDIO_V2_SIZEOF_AC_OUTPUT_TERMINAL_DESC +               \
	AUDIO_V2_SIZEOF_AC_CLOCK_SOURCE_DESC +                  \
	AUDIO_V2_SIZEOF_AC_INPUT_TERMINAL_DESC +                \
	AUDIO_V2_SIZEOF_AC_FEATURE_UNIT_DESC(IN_CHANNEL_NUM) +  \
	AUDIO_V2_SIZEOF_AC_OUTPUT_TERMINAL_DESC +               \
	0)

#define USB_CONFIG_SIZE (9 +                                \
	AUDIO_V2_AC_DESCRIPTOR_LEN +                            \
	CORE_DESCRIPTORS_SIZE +                                 \
	OUT_AS_DESCRIPTORS_LEN +                                \
	IN_AS_DESCRIPTORS_LEN +                                 \
	0)

#define AUDIO_AC_SIZ (AUDIO_V2_SIZEOF_AC_HEADER_DESC +      \
	CORE_DESCRIPTORS_SIZE +                                 \
	0)


#if   OUT_CHANNEL_NUM == 0
	#define IN_INTERFACE_ID               1
	#define OUT_INTERFACE_NUM             0
	#define OUT_AS_DESCRIPTORS_LEN        0
#else
	#define IN_INTERFACE_ID               2
	#define OUT_INTERFACE_NUM             1
	#if USING_FEEDBACK == 0
		#define OUT_AS_DESCRIPTORS_LEN    AUDIO_V2_AS_DESCRIPTOR_LEN
	#else
		#define OUT_AS_DESCRIPTORS_LEN    AUDIO_V2_AS_FEEDBACK_DESCRIPTOR_LEN
	#endif
#endif

#if   IN_CHANNEL_NUM == 0
	#define IN_INTERFACE_NUM              0
	#define IN_AS_DESCRIPTORS_LEN         0
#else
	#define IN_INTERFACE_NUM              1
	#define IN_AS_DESCRIPTORS_LEN         AUDIO_V2_AS_DESCRIPTOR_LEN
#endif

#define ALL_INTERFACES_NUM (1 + IN_INTERFACE_NUM + OUT_INTERFACE_NUM)



static const uint8_t device_descriptor[] = 
{
	USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0001, 0x01)
};

static const uint8_t config_descriptor[] = 
{
	USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, ALL_INTERFACES_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),                                            // 01.
	AUDIO_V2_AC_DESCRIPTOR_INIT(0x00, ALL_INTERFACES_NUM, AUDIO_AC_SIZ, AUDIO_CATEGORY_UNDEF, 0x00, 0x00),                                                    // 02.-04.
	AUDIO_V2_AC_CLOCK_SOURCE_DESCRIPTOR_INIT(AUDIO_OUT_CLOCK_ID, 0x03, 0x03),
	AUDIO_V2_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(0x02, AUDIO_TERMINAL_STREAMING, AUDIO_OUT_CLOCK_ID, OUT_CHANNEL_NUM, OUTPUT_CH_ENABLE, 0x0000),
	AUDIO_V2_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_OUT_FU_ID, 0x02, OUTPUT_CTRL),
	AUDIO_V2_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(0x04, AUDIO_OUTTERM_SPEAKER, AUDIO_OUT_FU_ID, AUDIO_OUT_CLOCK_ID, 0x0000),
	AUDIO_V2_AC_CLOCK_SOURCE_DESCRIPTOR_INIT(AUDIO_IN_CLOCK_ID, 0x03, 0x03),
	AUDIO_V2_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(0x06, AUDIO_INTERM_MIC, AUDIO_IN_CLOCK_ID, IN_CHANNEL_NUM, INPUT_CH_ENABLE, 0x0000),
	AUDIO_V2_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_IN_FU_ID, 0x06, INPUT_CTRL),
	AUDIO_V2_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(0x08, AUDIO_TERMINAL_STREAMING, AUDIO_IN_FU_ID, AUDIO_IN_CLOCK_ID, 0x0000),

	// Аудиовыход (SPK OUT)
#if   OUT_CHANNEL_NUM != 0
	#if USING_FEEDBACK == 0
		AUDIO_V2_AS_DESCRIPTOR_INIT         (0x01, 0x02, OUT_CHANNEL_NUM, OUTPUT_CH_ENABLE, AUDIO_SPEAKER_FRAME_SIZE_BYTE, AUDIO_SPEAKER_RESOLUTION_BIT, AUDIO_OUT_EP, 0x09, AUDIO_OUT_PACKET, EP_INTERVAL),
	#else
	//  AUDIO_V2_AS_FEEDBACK_DESCRIPTOR_INIT(bInterfaceNumber, bTerminalLink, bNrChannels,       bmChannelConfig,       bSubslotSize,                  bBitResolution,               bEndpointAddress,  wMaxPacketSize,   bInterval,   bFeedbackEndpointAddress) 
		AUDIO_V2_AS_FEEDBACK_DESCRIPTOR_INIT(0x01,             0x02,          OUT_CHANNEL_NUM,   OUTPUT_CH_ENABLE,      AUDIO_SPEAKER_FRAME_SIZE_BYTE, AUDIO_SPEAKER_RESOLUTION_BIT, AUDIO_OUT_EP,      AUDIO_OUT_PACKET, EP_INTERVAL, AUDIO_OUT_FEEDBACK_EP),
	#endif
#endif
	
#if   IN_CHANNEL_NUM != 0
		// Аудиовход (MIC IN)
	//  AUDIO_V2_AS_DESCRIPTOR_INIT(bInterfaceNumber, bTerminalLink, bNrChannels,      bmChannelConfig, bSubslotSize,              bBitResolution,           bEndpointAddress, bmAttributes, wMaxPacketSize,  bInterval)
		AUDIO_V2_AS_DESCRIPTOR_INIT(IN_INTERFACE_ID,  0x08,          IN_CHANNEL_NUM,   INPUT_CH_ENABLE, AUDIO_MIC_FRAME_SIZE_BYTE, AUDIO_MIC_RESOLUTION_BIT, AUDIO_IN_EP,      0x05,         AUDIO_IN_PACKET, EP_INTERVAL),
#endif
};


static const uint8_t device_quality_descriptor[] = 
{
	///////////////////////////////////////
	/// device qualifier descriptor
	///////////////////////////////////////
	0x0a,
	USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
	0x00,
	0x02,
	0x00,
	0x00,
	0x00,
	0x40,
	0x00,
	0x00,
};

static const char *string_descriptors[] = 
{
	(const char[]){0x09, 0x04},                                  /* Langid */
	"CherryUSB",				                                 /* Manufacturer */
	
	"CherryUAC " STR(UAC_BIT_RESOLUTION) "-"                   /* Product */
	#if   IN_CHANNEL_NUM != 0
		"M"  STR(IN_CHANNEL_NUM) "/" STR(AUDIO_IN_MAX_FREQ) "-"
	#endif
	#if   OUT_CHANNEL_NUM != 0
		"S"  STR(OUT_CHANNEL_NUM) "/" STR(AUDIO_OUT_MAX_FREQ)		
	#endif
	,
	"2022123456",			                                      /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
	return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
	return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
	return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
	if (index >= (sizeof(string_descriptors) / sizeof(char *)))
	{
		return NULL;
	}
	return string_descriptors[index];
}

const struct usb_descriptor audio_v2_descriptor = 
{
	.device_descriptor_callback = device_descriptor_callback,
	.config_descriptor_callback = config_descriptor_callback,
	.device_quality_descriptor_callback = device_quality_descriptor_callback,
	.string_descriptor_callback = string_descriptor_callback
};



static const uint8_t sampling_freq_table_8K[] = 
{
	AUDIO_SAMPLE_FREQ_NUM(1),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
};

static const uint8_t sampling_freq_table_16K[] = 
{
	AUDIO_SAMPLE_FREQ_NUM(2),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
};

static const uint8_t sampling_freq_table_32K[] = 
{
	AUDIO_SAMPLE_FREQ_NUM(3),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
};

static const uint8_t sampling_freq_table_48K[] = 
{
	AUDIO_SAMPLE_FREQ_NUM(4),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(48000),
	AUDIO_SAMPLE_FREQ_4B(48000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
};

static const uint8_t sampling_freq_table_96K[] = 
{
	AUDIO_SAMPLE_FREQ_NUM(5),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(48000),
	AUDIO_SAMPLE_FREQ_4B(48000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(96000),
	AUDIO_SAMPLE_FREQ_4B(96000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
};

static const uint8_t sampling_freq_table_192K[] = 
{
	AUDIO_SAMPLE_FREQ_NUM(6),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(8000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(16000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(24000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(48000),
	AUDIO_SAMPLE_FREQ_4B(48000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(96000),
	AUDIO_SAMPLE_FREQ_4B(96000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
	AUDIO_SAMPLE_FREQ_4B(192000),
	AUDIO_SAMPLE_FREQ_4B(192000),
	AUDIO_SAMPLE_FREQ_4B(0x00),
};

#if   AUDIO_IN_MAX_FREQ >= 192000
	#define mic_default_sampling_freq_table         sampling_freq_table_192K
#elif AUDIO_IN_MAX_FREQ >=  96000                
	#define mic_default_sampling_freq_table         sampling_freq_table_96K
#elif AUDIO_IN_MAX_FREQ >=  48000                
	#define mic_default_sampling_freq_table         sampling_freq_table_48K
#elif AUDIO_IN_MAX_FREQ >=  24000                
	#define mic_default_sampling_freq_table         sampling_freq_table_32K
#elif AUDIO_IN_MAX_FREQ >=  16000                
	#define mic_default_sampling_freq_table         sampling_freq_table_16K
#else                                            
	#define mic_default_sampling_freq_table         sampling_freq_table_8K
#endif

#if   AUDIO_OUT_MAX_FREQ >= 192000
	#define speaker_default_sampling_freq_table     sampling_freq_table_192K
#elif AUDIO_OUT_MAX_FREQ >=  96000
	#define speaker_default_sampling_freq_table     sampling_freq_table_96K
#elif AUDIO_OUT_MAX_FREQ >=  48000
	#define speaker_default_sampling_freq_table     sampling_freq_table_48K
#elif AUDIO_OUT_MAX_FREQ >=  24000
	#define speaker_default_sampling_freq_table     sampling_freq_table_32K
#elif AUDIO_OUT_MAX_FREQ >=  16000
	#define speaker_default_sampling_freq_table     sampling_freq_table_16K
#else
	#define speaker_default_sampling_freq_table     sampling_freq_table_8K
#endif

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t uac_read_buffer[AUDIO_OUT_PACKET];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t uac_write_buffer[AUDIO_IN_PACKET];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_speaker_feedback_buffer[4];

volatile bool rx_flag = 0;
volatile uint32_t s_mic_sample_rate;
volatile uint32_t s_speaker_sample_rate;
volatile uint32_t s_spk_bit_res = 0;

//volatile uint32_t s_mic_bit_res = 0;
//volatile uint32_t s_mic_ch_num  = 0;

volatile bool usb_is_ready = false; 
volatile TX_STATE_e g_USB_tx_state = TX_STATE_IDLE;
volatile int last_out_bytes = 0;

void generate_sine_wave16(uint8_t *buffer, uint32_t nbytes);
void generate_sine_wave24(uint8_t *buffer, uint32_t nbytes){}
void generate_sine_wave32(uint8_t *buffer, uint32_t nbytes){}
// void add_event(uint32_t val);
#define add_event(...)

//---------------------------------------------------------------------------------------
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
	int ch = ' ';
	switch (event)
	{
		case USBD_EVENT_RESET:
			usb_is_ready = false;	
			ch = 'R';			
		break;
		case USBD_EVENT_CONNECTED:
			ch = 'C';			
		break;
		case USBD_EVENT_DISCONNECTED:
			usb_is_ready = false;
			ch = 'D';			
		break;
		case USBD_EVENT_RESUME:
			ch = 'M';			
		break;
		case USBD_EVENT_SUSPEND:
			ch = 'P';			
		break;
		case USBD_EVENT_CONFIGURED:
			usb_is_ready = true;
			ch = 'U';			
		break;
		case USBD_EVENT_SET_REMOTE_WAKEUP:
			ch = 'W';			
		break;
		case USBD_EVENT_CLR_REMOTE_WAKEUP:
			ch = 'w';			
		break;

		default:
			ch = '?';			
		break;
	}
	add_event('E' | ((uint32_t)event << 8) | (ch << 16) | 0x40000000);
}

//---------------------------------------------------------------------------------------
void usbd_audio_open(uint8_t busid, uint8_t intf)
{
	if (intf == IN_INTERFACE_ID)
	{
		//tx_flag = 1;
		g_USB_tx_state = TX_STATE_STARTED;
		USB_LOG_RAW("OPEN2\r\n");
	}
	else
	{
		rx_flag = 1;
		/* setup first out ep read transfer */
		usbd_ep_start_read(busid, AUDIO_OUT_EP, uac_read_buffer, AUDIO_OUT_PACKET);
		#if USING_FEEDBACK == 1
		#ifdef CONFIG_USB_HS
				uint32_t feedback_value = AUDIO_FREQ_TO_FEEDBACK_HS(AUDIO_FREQ);		
				AUDIO_FEEDBACK_TO_BUF_HS(s_speaker_feedback_buffer, feedback_value);
		#else
				//uint32_t feedback_value = AUDIO_FREQ_TO_FEEDBACK_FS(AUDIO_FREQ);
				uint32_t feedback_value = AUDIO_FREQ_TO_FEEDBACK_10_14(s_speaker_sample_rate);
				AUDIO_FEEDBACK_TO_BUF_FS(s_speaker_feedback_buffer, feedback_value);
		#endif
				usbd_ep_start_write(busid, AUDIO_OUT_FEEDBACK_EP, s_speaker_feedback_buffer, FEEDBACK_ENDP_PACKET_SIZE);
		#endif
		USB_LOG_RAW("OPEN1\r\n");
	}
	add_event('O' | ((uint32_t)intf << 8));
}

//---------------------------------------------------------------------------------------
void usbd_audio_close(uint8_t busid, uint8_t intf)
{
	if (intf == IN_INTERFACE_ID)
	{
		//tx_flag = 0;
		g_USB_tx_state = TX_STATE_IDLE;
		USB_LOG_RAW("CLOSE2\r\n");
	}
	else
	{
		rx_flag = 0;
		USB_LOG_RAW("CLOSE1\r\n");
	}
	add_event('C' | ((uint32_t)intf << 8));
}

//---------------------------------------------------------------------------------------
void usbd_audio_set_sampling_freq(uint8_t busid, uint8_t ep, uint32_t sampling_freq)
{
	if (ep == AUDIO_OUT_EP)
	{
		s_speaker_sample_rate = sampling_freq;
	}
	else if (ep == AUDIO_IN_EP)
	{
		s_mic_sample_rate = sampling_freq;
	}
	add_event('F' | ((uint32_t)ep << 8) | ((sampling_freq / 1000) << 16) | 0x80000000);
}

//---------------------------------------------------------------------------------------
uint32_t usbd_audio_get_sampling_freq(uint8_t busid, uint8_t ep)
{
	(void)busid;

	uint32_t freq = 0;

	if (ep == AUDIO_OUT_EP)
	{
		freq = s_speaker_sample_rate;
	}
	else if (ep == AUDIO_IN_EP)
	{
		freq = s_mic_sample_rate;
	}
	add_event('G' | ((uint32_t)ep << 8) | ((freq / 1000) << 16) | 0x80000000);
	return freq;
}

//---------------------------------------------------------------------------------------
void usbd_audio_get_sampling_freq_table(uint8_t busid, uint8_t ep, uint8_t **sampling_freq_table)
{
	if (ep == AUDIO_OUT_EP)
	{
		*sampling_freq_table = (uint8_t *)speaker_default_sampling_freq_table;
	}
	else if (ep == AUDIO_IN_EP)
	{
		*sampling_freq_table = (uint8_t *)mic_default_sampling_freq_table;
	}
	else
	{
	}
	add_event('T' | ((uint32_t)ep << 8));
}

//---------------------------------------------------------------------------------------
void usbd_audio_iso_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	USB_LOG_RAW("actual out len:%d\r\n", (unsigned int)nbytes);
	usbd_ep_start_read(busid, AUDIO_OUT_EP, uac_read_buffer, AUDIO_OUT_PACKET);
	add_event('<' | (nbytes << 8) | 0x20000000);
	last_out_bytes = nbytes;
}

//---------------------------------------------------------------------------------------
void usbd_audio_iso_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	USB_LOG_RAW("actual in len:%d\r\n", (unsigned int)nbytes);
	g_USB_tx_state = TX_STATE_CB_COMPLETE;
	//ep_tx_busy_flag = false;
}

//---------------------------------------------------------------------------------------
#if USING_FEEDBACK == 1
	void usbd_audio_iso_out_feedback_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
	{
		USB_LOG_RAW("actual feedback len:%d\r\n", nbytes);
		uint32_t feedback_value;
		#ifdef CONFIG_USB_HS
			feedback_value = AUDIO_FREQ_TO_FEEDBACK_HS(s_speaker_sample_rate);	
			AUDIO_FEEDBACK_TO_BUF_HS(s_speaker_feedback_buffer, feedback_value);
		#else
			//uint32_t feedback_value = AUDIO_FREQ_TO_FEEDBACK_FS(s_speaker_sample_rate);
			//uint32_t feedback_value = AUDIO_FREQ_TO_FEEDBACK_10_14(s_speaker_sample_rate);
			//AUDIO_FEEDBACK_TO_BUF_FS(s_speaker_feedback_buffer, feedback_value);

			#if OUT_CHANNEL_NUM > 0
				int curr_speed = last_out_bytes * 1000;
				
				curr_speed /= AUDIO_SPEAKER_FRAME_SIZE_BYTE * OUT_CHANNEL_NUM;
				
				curr_speed -= s_speaker_sample_rate;
				
				curr_speed >>= 16;
				
				int fb_speed = s_speaker_sample_rate - curr_speed;
				
				feedback_value = AUDIO_FREQ_TO_FEEDBACK_10_14(fb_speed);
				AUDIO_FEEDBACK_TO_BUF_FS(s_speaker_feedback_buffer, feedback_value);
			#endif
			
		#endif
		usbd_ep_start_write(busid, AUDIO_OUT_FEEDBACK_EP, s_speaker_feedback_buffer, FEEDBACK_ENDP_PACKET_SIZE);
		add_event((feedback_value & 0x00ffffff) | 0x10000000);
	}
#endif

//---------------------------------------------------------------------------------------
static struct usbd_endpoint audio_out_ep = 
{
	.ep_cb = usbd_audio_iso_out_callback,
	.ep_addr = AUDIO_OUT_EP};

static struct usbd_endpoint audio_in_ep = 
{
	.ep_cb = usbd_audio_iso_in_callback,
	.ep_addr = AUDIO_IN_EP};

#if USING_FEEDBACK == 1
static struct usbd_endpoint audio_out_feedback_ep = 
{
	.ep_cb = usbd_audio_iso_out_feedback_callback,
	.ep_addr = AUDIO_OUT_FEEDBACK_EP};
#endif

struct usbd_interface intf0;
struct usbd_interface intf1;
struct usbd_interface intf2;

struct audio_entity_info audio_entity_table[] = 
{
	{.bEntityId = AUDIO_OUT_CLOCK_ID,
		.bDescriptorSubtype = AUDIO_CONTROL_CLOCK_SOURCE,
		.ep = AUDIO_OUT_EP},
	{.bEntityId = AUDIO_OUT_FU_ID,
		.bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT,
		.ep = AUDIO_OUT_EP},
	{.bEntityId = AUDIO_IN_CLOCK_ID,
		.bDescriptorSubtype = AUDIO_CONTROL_CLOCK_SOURCE,
		.ep = AUDIO_IN_EP},
	{.bEntityId = AUDIO_IN_FU_ID,
		.bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT,
		.ep = AUDIO_IN_EP},
};
//---------------------------------------------------------------------------------------

// In windows, audio driver cannot remove auto, so when you modify any descriptor information, please modify string descriptors too.

void audio_v2_init(uint8_t busid, uintptr_t reg_base)
{
	usbd_desc_register(busid, &audio_v2_descriptor);

	usbd_add_interface(busid, usbd_audio_init_intf(busid, &intf0, 0x0200, audio_entity_table, 4));
	usbd_add_interface(busid, usbd_audio_init_intf(busid, &intf1, 0x0200, audio_entity_table, 4));
	usbd_add_interface(busid, usbd_audio_init_intf(busid, &intf2, 0x0200, audio_entity_table, 4));
	usbd_add_endpoint(busid, &audio_in_ep);
	usbd_add_endpoint(busid, &audio_out_ep);
#if USING_FEEDBACK == 1
	usbd_add_endpoint(busid, &audio_out_feedback_ep);
#endif

	usbd_initialize(busid, reg_base, usbd_event_handler);
}
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
void audio_send_mic_buff(uint8_t busid, int bytes_to_send)
{
	if(g_USB_tx_state == TX_STATE_CB_COMPLETE || g_USB_tx_state == TX_STATE_STARTED)
	{
		g_USB_tx_state = TX_STATE_CB_WAIT;
		

		usbd_ep_start_write(busid, AUDIO_IN_EP, uac_write_buffer, bytes_to_send);
	}
}
//---------------------------------------------------------------------------------------
int audio_generate_mic_buff(void)
{
	// Вычисляем, сколько байт нужно передать для текущей частоты в 1		
	int mic_bits = AUDIO_MIC_RESOLUTION_BIT; // s_mic_bit_res;
	int mic_chnn = IN_CHANNEL_NUM; // s_mic_ch_num;
	uint32_t samples_to_send = (s_mic_sample_rate * mic_chnn) / 1000;
	
	if(samples_to_send > sizeof(uac_write_buffer)) samples_to_send = sizeof(uac_write_buffer);

	int bytes_to_send = 0;

	// Генерируем синус прямо в буфер отправки
	if(mic_bits == 16)
	{
		bytes_to_send = samples_to_send * 2;
		generate_sine_wave16(uac_write_buffer, bytes_to_send);
	}
	else if(mic_bits == 24)
	{
		bytes_to_send = samples_to_send * 3;
		generate_sine_wave24(uac_write_buffer, bytes_to_send);
	}
	else if(mic_bits == 32)
	{
		bytes_to_send = samples_to_send * 4;
		generate_sine_wave32(uac_write_buffer, bytes_to_send);
	}
	return bytes_to_send;
}
//---------------------------------------------------------------------------------------
void audio_v2_test(uint8_t busid)
{
	if(g_USB_tx_state == TX_STATE_CB_COMPLETE || g_USB_tx_state == TX_STATE_STARTED)
	{
		audio_send_mic_buff(busid, audio_generate_mic_buff());
	}
	// if (tx_flag)
	// {
		// memset(uac_write_buffer, 'a', AUDIO_IN_PACKET);
		// ep_tx_busy_flag = true;
		// usbd_ep_start_write(busid, AUDIO_IN_EP, uac_write_buffer, AUDIO_IN_PACKET);
		// while (ep_tx_busy_flag)
		// {
			// if (tx_flag == false)
			// {
				// break;
			// }
		// }
	// }
	// if (rx_flag)
	// {
	// }
}
//---------------------------------------------------------------------------------------
