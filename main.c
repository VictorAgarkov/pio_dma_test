/*
	Попытка воспроизвести глюк DMA, условия следующие:
	
	- PIO получает данные из ОЗУ по DMA (отдаёт их наружу)
	- main в это время читает что-то из flash через XIP
	
	Сам глюк выглядит так:
	если в момент, когда идёт чтение из flash (шина находится 
	в одидании), возникает запрос DMA от FIFO PIO, то в какой-то 
	момент происходит переполнение этого FIFO (типа DMA 
	записывает в него не 1, а 2 слова по одному запросу).
	
	Результаты изысканий:
	- не зависит от работы XIP
	- происходит только при первой пересылке буфера
	- бит TXOVER взводится несколько раз: при размере буфера 64 слова 
	  и глубине FIFO 8 слов число ошибок равно 64-8=56
	- не зависит, разбит ли буфер на две половинки или каждым каналом 
	  DMA выводится целиком

*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"  
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "dma_test.pio.h"  // PIO program

#define LEDPIN    PICO_DEFAULT_LED_PIN
#define DBGPIN1   13   // core 0 run
#define DBGPIN2   14   // core 1 run
#define DBGPIN3   15   // PIO data output
#define DBGPIN4   16   // within DMA_IRQ0_HANDLER 
#define DBGPIN5   17   // TXOVER in FDEBUG detected


PIO dma_pio = pio0;

volatile uint32_t dma_buff[32];
volatile int g_core1_rdy = 0;

#ifndef ARRAYSIZE
	#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
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
void set_dma_src_addr(int half)
{
	int offs = half ? ARRAYSIZE(dma_buff) / 2 : 0;
	dma_channel_set_read_addr(half, dma_buff + offs, false);			
}
//------------------------------------------------------------------------------------------------------------------------------------------------
void setup_DMA_tx(PIO pio, uint sm, int dma_chn, int dma_chn_next, uint buff_size)
{
	// channel config
	dma_channel_config config = dma_channel_get_default_config(dma_chn);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_32); 
	channel_config_set_read_increment    (&config, true);            
	channel_config_set_write_increment   (&config, false);          
	channel_config_set_chain_to          (&config, dma_chn_next);              
	channel_config_set_dreq              (&config, pio_get_dreq(pio, sm, true));
	channel_config_set_irq_quiet         (&config, false);

	// setup, but not start
	dma_channel_hw_t *dma_p = dma_channel_hw_addr(dma_chn);
	set_dma_src_addr(dma_chn);
	//dma_p->read_addr        = (uintptr_t) buff;
	dma_p->write_addr       = (uintptr_t) &pio->txf[sm];
	dma_p->transfer_count   = buff_size;
	dma_p->al1_ctrl         = channel_config_get_ctrl_value(&config);
	
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
			set_dma_src_addr(i);			
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
		sio_hw->gpio_togl = (1 << DBGPIN2);		// toggle pin - core 1 running		
		uint32_t fdb = dma_pio->fdebug;
		gpio_put(DBGPIN5, fdb == 0x00010000);  // set pin, if TXOVER0 raised
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
	for(int i = 0; i < ARRAYSIZE(dma_buff); i++)
	{
		dma_buff[i] = 0x000000ff | ((i + 1) << 9);
	}

	// init core 1
	multicore_reset_core1(); 
	sleep_ms(10);
	multicore_launch_core1(core1_routine); // start core1
	while(!g_core1_rdy);
	
	//init PIO
	uint offset = pio_add_program(dma_pio, &dma_test_pio_program);
	uint sm = pio_claim_unused_sm(dma_pio, true);
	dma_test_pio_program_init    (dma_pio, sm, offset, DBGPIN3);
	pio_sm_set_enabled(dma_pio, sm, true);     // start PIO
	
	// init DMA for PIO	
	setup_DMA_tx(dma_pio, sm, 0, 1, ARRAYSIZE(dma_buff) / 2);
	setup_DMA_tx(dma_pio, sm, 1, 0, ARRAYSIZE(dma_buff) / 2);
	dma_start_channel_mask(1 | 2);             // start both DMA

	sio_hw->gpio_set = (1 << LEDPIN);
	
	for(;;)
	{
		
		volatile uint32_t sum = 0;
		{
			sio_hw->gpio_togl = (1 << DBGPIN1); // toggle oin - core 0 running
		}
	}	
}
//------------------------------------------------------------------------------------------------------------------------------------------------
