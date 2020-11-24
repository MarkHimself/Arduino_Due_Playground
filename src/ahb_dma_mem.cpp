#include "ahb_dma_mem.h"

// setup instructions on pg. 346

uint8_t ch_num = 0;

bool setup_AHB_DMA_mem_to_mem(){
	uint32_t temp_var = 0;
	// setup clock for AHB DMA.
	PMC->PMC_PCER1 = PMC_PCER1_PID39;
	DMAC->DMAC_WPMR = 0x444D41 << 8;	// unlock write protect					pg. 380
	DMAC->DMAC_EN = 0;					// disable DMA Controller				pg. 360
	temp_var = DMAC->DMAC_EBCISR;		// read status of dma to clear int's.	pg. 367
	// find an unused channel.
	temp_var = DMAC->DMAC_CHSR;			// Channel handler status register		pg. 370
	// find a disabled channel.
	for (uint8_t i = 0, en_mask = 1; i < 6; i++, en_mask = en_mask << 1){
		if (temp_var & en_mask){
			if (i == 5) return false;		// couldn't find a disabled channel
		}
		else{
			ch_num = i;
			break;
		}
	}
	
	
	
	DMAC->DMAC_CH_NUM[ch_num].DMAC_DSCR = 0;
	return true;
}

void start_AHB_DMA_mem_to_mem(char *source, char *dest, uint8_t str_length){
	DMAC->DMAC_CH_NUM[ch_num].DMAC_SADDR = (uint32_t)source;	// source address		pg. 371
	DMAC->DMAC_CH_NUM[ch_num].DMAC_DADDR = (uint32_t)dest;		// destination address	pg. 372
	DMAC->DMAC_CH_NUM[ch_num].DMAC_DSCR = 0;		// Next descriptor address is none	pg. 373
	
	DMAC->DMAC_CH_NUM[ch_num].DMAC_CTRLA = 0		// control A				pg. 374
		| DMAC_CTRLA_BTSIZE(str_length)				// transfer to do
		| DMAC_CTRLA_SRC_WIDTH_BYTE					// transfer bytes.
		| DMAC_CTRLA_DST_WIDTH_BYTE					// receive bytes.
	;
	
	DMAC->DMAC_CH_NUM[ch_num].DMAC_CTRLB = 0		// control B				pg. 376
		| DMAC_CTRLB_FC_MEM2MEM_DMA_FC				// mem to mem
		| DMAC_CTRLB_SRC_INCR_INCREMENTING			// increment source address
		| DMAC_CTRLB_DST_INCR_INCREMENTING			// increment destination address
	;
	
	DMAC->DMAC_CH_NUM[ch_num].DMAC_CFG = 0	// configuration register			pg. 378
		| DMAC_CFG_SRC_H2SEL_SW				// software handshaking
		| DMAC_CFG_DST_H2SEL_SW				// software handshaking
	;
	
	DMAC->DMAC_EN = DMAC_EN_ENABLE;			// enable DMAC						pg. 360
	DMAC->DMAC_EN = DMAC->DMAC_CHER = (1 << ch_num);	// enable the channel			pg. 368
	
	while (DMAC->DMAC_CHSR & (1 << ch_num)){			// while ch. is enabled	pg. 370
		// wait for DMA to finish.
	}
	
}









