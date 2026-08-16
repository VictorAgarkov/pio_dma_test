#include <stdio.h>
#include "RP2040.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"  
#include "hardware/dma.h"
#include "hardware/clocks.h"


#include "dma_test.pio.h"  // PIO program
#include "uac2.h"

#define LEDPIN    PICO_DEFAULT_LED_PIN
#define DBGPIN1   13   // core 0 run
#define DBGPIN2   14   // USB IRQ (see ../CherryUSB/port/rp2040/usb_dc_rp2040.c)
#define DBGPIN3   15   // core 1 running
#define DBGPIN4   16   // within DMA_IRQ0_HANDLER 
#define DBGPIN5   17   // TXOVER in FDEBUG detected


PIO dma_pio = pio0;

volatile uint32_t dma_buff_A[16*2]  __attribute__((aligned(16*4)));
volatile uint32_t dma_buff_B[16*2]  __attribute__((aligned(16*4)));
volatile int g_core1_rdy = 0;

#ifndef ARRAYSIZE
	#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define HALF_BUFF_SIZE (ARRAYSIZE(dma_buff_A) / 2)

int32_t get_sine_int32(uint32_t angle);
uint32_t sine_freq = 1222.2 / 48000 * 0x100000000ULL;
uint32_t sine_phase = 0;

extern volatile bool usb_is_ready; 

//------------------------------------------------------------------------------------------------------------------------------------------------
void init_out_pins(int pins[], int num)
{
	for(int i = 0; i < num; i++)
	{
		gpio_init(pins[i]);
		gpio_set_dir(pins[i], GPIO_OUT);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------
void generate_sine_wave16(uint8_t *buffer, uint32_t nbytes)
{
	int16_t *pcm_buffer = (int16_t *)buffer;
	int samples_count = nbytes / 2 / IN_CHANNEL_NUM;
	for(int i = 0; i < samples_count; i++)
	{
		uint32_t dphase = 0;
		for(int j = 0; j < IN_CHANNEL_NUM; j++)
		{
			*(pcm_buffer++) = get_sine_int32(sine_phase + dphase) >> 16;
			dphase += 0x80000000 / IN_CHANNEL_NUM;
		}
		sine_phase += sine_freq;
	}	
};
//------------------------------------------------------------------------------------------------------------------------------------------------
void setup_DMA_tx(PIO pio, uint sm, int dma_chn, int dma_chn_next, volatile void *buff, uint buff_size)
{
	// channel config
	dma_channel_config config = dma_channel_get_default_config(dma_chn);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_32); 
	channel_config_set_read_increment    (&config, true);            
	channel_config_set_write_increment   (&config, false);          
	channel_config_set_chain_to          (&config, dma_chn_next);              
	channel_config_set_dreq              (&config, pio_get_dreq(pio, sm, true));
	channel_config_set_irq_quiet         (&config, false);
	channel_config_set_ring              (&config, 0, 6); // ring buff 64 bytes (16w) on read address

	// setup, but not start
	dma_channel_hw_t *dma_p = dma_channel_hw_addr(dma_chn);
	dma_p->write_addr       = (uintptr_t) &pio->txf[sm];
	dma_p->transfer_count   = buff_size;
	dma_p->al1_ctrl         = channel_config_get_ctrl_value(&config);
	dma_p->read_addr        = (uintptr_t) buff;
	
	dma_channel_set_irq0_enabled(dma_chn, true);	
}
//------------------------------------------------------------------------------------------------------------------------------------------------
void enable_DMA_IRQ(uint num, irq_handler_t handler)
{
	irq_set_exclusive_handler(num, handler);	
	irq_set_enabled(num, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------------
__attribute__((noinline, section(".scratch_x.rb32_functions")))
void dma_irq0_handler(void)
{
	gpio_put(DBGPIN4, 1);
	
	// find channel
	for(int i = 0; i < 2; i++)
	{
		int chn_mask = 1 << i;		
		
		if (dma_hw->ints0 & chn_mask)
		{
			dma_hw->ints0 = chn_mask;	
			//set_dma_src_addr(i);			
		}			
	}
	gpio_put(DBGPIN4, 0);
}
//------------------------------------------------------------------------------------------------------------------------------------------------
__attribute__((noinline, section(".scratch_x.rb32_functions"))) // effect
void core1_routine(void)
{
	enable_DMA_IRQ(DMA_IRQ_0, dma_irq0_handler);	// enable IRQ from core 1

	g_core1_rdy = 1;
	for(;;)
	{	
		sio_hw->gpio_togl = (1 << DBGPIN3);		// toggle pin - core 1 running		
		uint32_t fdb = dma_pio->fdebug;
		//gpio_put(DBGPIN5, fdb == 0x00010000);  // set pin, if TXOVER0 raised
		gpio_put(DBGPIN5, fdb != 0);  // set pin, if some bit in FDEBUG rized
		if(fdb) 
		{			
			dma_pio->fdebug = fdb; // reset errors
		}
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------
int main(void)
{
	set_sys_clock_khz(100000, true);
	stdio_init_all();
	
	init_out_pins((int[]){LEDPIN, DBGPIN1, DBGPIN2, DBGPIN3, DBGPIN4, DBGPIN5}, 6);
	
	// init output sequence
	for(int i = 0; i < ARRAYSIZE(dma_buff_A); i++)
	{
		dma_buff_A[i] = 0x000000ff | ((i + 1) << 9);
		dma_buff_B[i] = 0xf0f00000 | ((i + 1) << 9);
	}

	// init core 1
	multicore_reset_core1(); 
	sleep_ms(10);
	multicore_launch_core1(core1_routine); // start core1
	while(!g_core1_rdy);
	
	//init PIO
	uint offset = pio_add_program(dma_pio, &dma_test_pio_program);
	
	uint sm_A = pio_claim_unused_sm(dma_pio, true);
	dma_test_pio_program_init    (dma_pio, sm_A, offset, 1); // start 1st transmitter on pin #1
	
	uint sm_B = pio_claim_unused_sm(dma_pio, true);
	dma_test_pio_program_init    (dma_pio, sm_B, offset, 2); // start 2nd transmitter on pin #2
	
	// init DMA for PIO:
	// channles 0/1 - buffer A
	// channles 2/3 - buffer B
	setup_DMA_tx(dma_pio, sm_A, 0, 1, dma_buff_A + 0,              HALF_BUFF_SIZE);
	setup_DMA_tx(dma_pio, sm_A, 1, 0, dma_buff_A + HALF_BUFF_SIZE, HALF_BUFF_SIZE);
	setup_DMA_tx(dma_pio, sm_B, 2, 3, dma_buff_B + 0,              HALF_BUFF_SIZE);
	setup_DMA_tx(dma_pio, sm_B, 3, 2, dma_buff_B + HALF_BUFF_SIZE, HALF_BUFF_SIZE);

	dma_start_channel_mask((1 << 0) | (1 << 2));        // start first half of DMA 

	pio_set_sm_mask_enabled(dma_pio, (1 << sm_A) | (1 << sm_B), 1); // start PIO
	
	// init USB audio
	audio_v2_init(0, USB_BASE);	
	
	for(;;)
	{
		gpio_put(LEDPIN, usb_is_ready);
		
		volatile uint32_t sum = 0;
		uint32_t *base32 = (uint32_t*)XIP_MAIN_BASE; // XIP_MAIN_BASE, XIP_NOALLOC_BASE, XIP_NOCACHE_BASE и XIP_NOCACHE_NOALLOC_BASE
//		uint8_t  *base8  =  (uint8_t*)XIP_MAIN_BASE; // XIP_MAIN_BASE, XIP_NOALLOC_BASE, XIP_NOCACHE_BASE и XIP_NOCACHE_NOALLOC_BASE
//		for(int i = 0; i < 0x00400000; i++)
		{
//			//sum += base8[i];
//			sum += base32[i];
			sio_hw->gpio_togl = (1 << DBGPIN1); // toggle oin - core 0 running
		}
		
		// UAC routine
		audio_v2_test(0);
		
	}	
}
//------------------------------------------------------------------------------------------------------------------------------------------------
