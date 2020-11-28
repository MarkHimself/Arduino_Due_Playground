#include "uart.h"

void setup_UART(){
	PMC->PMC_PCER0 = PMC_PCER0_PID8;	// clock for UART						pg. 38, 542
	UART->UART_CR = 0			// Control Register								pg. 759
		| UART_CR_RSTRX			// reset receiver
		| UART_CR_RSTTX			// reset transmitter
		| UART_CR_RSTSTA		// reset status bits
	;
	UART->UART_MR = 0			// Mode Register								pg. 760
		| UART_MR_PAR_NO		// no parity
		| UART_MR_CHMODE_NORMAL	// normal
	;
	UART->UART_IDR = (0xFFFF);	// disable all interrupts						pg. 762
	// baud rate chosen by:	MCK / (16 * CD)
	UART->UART_BRGR = 137;		// baud rate generator registor					pg. 768
	UART->UART_CR = 0			// Control Register								pg. 759
		| UART_CR_TXEN			// enable transmitter
	;
}

bool transmit_UART(uint8_t val){
	
	if (!(UART->UART_SR & UART_SR_TXRDY)) return false;	// if transmitter not empty		pg. 764
	UART->UART_THR = val;		// load the transmitter holding register		pg. 767
	
	return true;
}

void setup_PIOA8_as_UART_RX(){
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// clock for PIOA						pg. 38, 542
	PIOA->PIO_WPMR = 0x50494F << 8;		// unlock write protect					pg. 674
	PIOA->PIO_PUDR = PIO_PUDR_P8;		// disable pull up resistor				pg. 622, 
	PIOA->PIO_PDR = PIO_PDR_P8;		// disable PIO control of pin				pg. 622, 
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P8);	// select peripheral A	pg. 622, 
}

void setup_PIOA9_as_UART_TX(){
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// clock for PIOA						pg. 38, 542
	PIOA->PIO_WPMR = 0x50494F << 8;		// unlock write protect					pg. 674
	PIOA->PIO_PUDR = PIO_PUDR_P9;		// disable pull up resistor				pg. 622, 
	PIOA->PIO_PDR = PIO_PDR_P9;		// disable PIO control of pin				pg. 622, 
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P9);	// select peripheral A	pg. 622, 
}

void setup_UART_empty_tx_int(){
	// either TXWMPTY or TXRDY work. TXRDY is used on DMA though.
	//UART->UART_IER = UART_IER_TXEMPTY;	// create an interrupt when empty	pg. 761
	UART->UART_IER = UART_IER_TXRDY;		// create an interrupt when TX ready	pg. 761
	NVIC_EnableIRQ(UART_IRQn);			// nvic functions						pg. 164
	NVIC_SetPriority(UART_IRQn, 4);
}

void setup_UART_TX_String_DMA(){
	
	UART->UART_PTCR = 0		// transfer control register						pg. 517
		| UART_PTCR_RXTDIS	// Receiver transfer disable
		| UART_PTCR_TXTDIS	// Transmitter transfer disable
	;
	
	// receive = write	
	UART->UART_RPR = 0;								// receive buffer address	pg. 509
	UART->UART_RCR = 0;								// receiver buffer size		pg. 510
	
	// transmit = read
	UART->UART_TPR = 0;								// transmit the string		pg. 511
	UART->UART_TCR = 0;								// transmit length			pg. 512
	
	// receive = write next 
	UART->UART_RNPR = 0;		// receive next pointer							pg. 513
	UART->UART_RNCR = 0;		// receive next counter							pg. 514
	
	// transmit = read next
	UART->UART_TNPR = 0;		// transmit next pointer						pg. 515
	UART->UART_TNCR = 0;		// transmit next counter						pg. 516
}

void start_UART_TX_String_DMA_Transfer(char *str, uint8_t str_length){
	
	// receive = write	
	UART->UART_RPR = (uint32_t)&(UART->UART_THR);	// receive buffer address	pg. 509
	UART->UART_RCR = 1;								// receiver buffer size		pg. 510
	
	// transmit = read
	UART->UART_TPR = (uintptr_t)str;				// transmit the string		pg. 511
	UART->UART_TCR = str_length;					// transmit length			pg. 512
	
	UART->UART_PTCR = 0				// transfer control register				pg. 517
		| UART_PTCR_RXTEN
		| UART_PTCR_TXTEN
	;
}


// *** USART0 setup ***
// USART0 is instance number 17

void setup_USART0(uint32_t baud_rate){
	PMC->PMC_PCER0 = PMC_PCER0_PID17;		// clock for USART0					pg. 38, 
	USART0->US_WPMR = 0x555341 << 8;		// unlock write protect				pg. 854
	USART0->US_CR = 0		// control reg.										pg. 824
		| US_CR_RSTRX		// Reset receiver
		| US_CR_RSTTX		// Reset transmitter
		| US_CR_RSTSTA		// reset status bits
	;
	USART0->US_MR = 0						// mode reg							pg. 827
		| US_MR_USART_MODE_NORMAL			// normal mode
		| US_MR_USCLKS_MCK					// master clock
		| US_MR_CHRL_8_BIT
		| (0x0 << 8)						// async mode
		| US_MR_PAR_NO
		| US_MR_NBSTOP_1_BIT
		| US_MR_CHMODE_NORMAL
		| US_MR_MSBF
		| US_MR_OVER						// oversampling mode. yeak ok. baud rate depends on this bit.
	;
	if (USART0->US_MR & US_MR_OVER){		// since baud rate depends on this bit.
		USART0->US_BRGR = 0						// baud rate generator				pg. 843
			| US_BRGR_FP(2)						// resolution. ??
			| US_BRGR_CD(84000000 / (8 * baud_rate))
		;
	}
	else{
		USART0->US_BRGR = 0						// baud rate generator				pg. 843
			| US_BRGR_FP(2)						// resolution. ??
			| US_BRGR_CD(84000000 / (16 * baud_rate))
		;
	}
	USART0->US_CR = US_CR_TXEN;				// enable transmitter				pg. 824
}

// PIOA11 is TXD0 on peripheral A. is Arduino TX1 (d.p. 18)
void setup_PIOA11_as_USART0_TX(){
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// clock for PIOA						pg. 38, 542
	PIOA->PIO_WPMR = 0x50494F << 8;		// unlock write protect					pg. 674
	PIOA->PIO_PUDR = PIO_PUDR_P11;		// disable pull up resistor				pg. 622, 
	PIOA->PIO_PDR = PIO_PDR_P11;		// disable PIO control of pin			pg. 622, 
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P11);	// select peripheral A	pg. 622, 
}

bool write_USART0_TX(uint8_t val){
	
	if (!(USART0->US_CSR & US_CSR_TXRDY)) return false;	// if transmitter not empty		pg. 837
	USART0->US_THR = val;		// load the transmitter holding register		pg. 842
	
	return true;
}




/*
Will use AHB DMA Ch. 1
*/
void setup_USART0_TX_String_AHB_DMA(){
	uint32_t temp = 0;
	PMC->PMC_PCER1 = PMC_PCER1_PID39;	// clock for DMAC						pg. 39, 563
	DMAC->DMAC_WPMR =  0x444D41 << 8;	// unlock write protect.				pg. 380
	temp = DMAC->DMAC_WPSR;				// clear any potential errors			pg. 381
	DMAC->DMAC_EN = DMAC_EN_ENABLE;		// enable DMA controller				pg. 360
	
	DMAC->DMAC_CHDR = DMAC_CHDR_DIS1;	// disable ch. 1						pg. 369
	while(DMAC->DMAC_CHSR & DMAC_CHSR_ENA1){};	// wait while channel still enabled.	pg. 370
	DMAC->DMAC_EBCIDR = 0				// disable interrupts on ch. 1			pg. 365
		| DMAC_EBCIDR_BTC1
		| DMAC_EBCIDR_CBTC1
		| DMAC_EBCIDR_ERR1
	;
	temp = DMAC->DMAC_EBCISR;			// read status of ints					pg. 367
	DMAC->DMAC_CH_NUM[1].DMAC_DSCR = 0;	// descriptor = 0						pg. 373
}

void start_USART0_TX_String_AHB_DMA(char *str, uint8_t str_length){
	
	// return if an ongoing transaction exists.
	if (DMAC->DMAC_CHSR & DMAC_CHSR_ENA1) return;		// return if channel enabled. pg. 370
	
	DMAC->DMAC_CH_NUM[1].DMAC_SADDR = (uint32_t)str;				// source address		pg. 371
	DMAC->DMAC_CH_NUM[1].DMAC_DADDR = (uint32_t)&(USART0->US_THR);	// destination			pg. 372
	DMAC->DMAC_CH_NUM[1].DMAC_DSCR = 0;	// descriptor = 0						pg. 373
	
	DMAC->DMAC_CH_NUM[1].DMAC_CTRLA = 0		// control A register				pg. 374
		| DMAC_CTRLA_BTSIZE(str_length)		// length of string.
		| DMAC_CTRLA_SCSIZE_CHK_1			// ???		// results in 32.58 ms between transmissions (with the 50 ms delay in loop())
		| DMAC_CTRLA_DCSIZE_CHK_1			// ???
		//| DMAC_CTRLA_SCSIZE_CHK_4			// ???		// results in 45.59 ms between transmission
		//| DMAC_CTRLA_DCSIZE_CHK_4			// ???
		//| DMAC_CTRLA_SCSIZE_CHK_8			// ???		// results in 47.68 ms between transmission
		//| DMAC_CTRLA_DCSIZE_CHK_8			// ???
		| DMAC_CTRLA_SRC_WIDTH_BYTE
		| DMAC_CTRLA_DST_WIDTH_BYTE
	;
	/*
	CHK_1:
	works.
	
	CHK_4:
	37bfjnrvz+DHLPTXO	(don't know what that last O is)
	looks like it transmits every 4th char.
	
	CHK_8:
	only 8 bytes are transmitted every time this function is called.
	somehow the bytes that are transmitted are like every 8 bytes in the string or so.
	7fnv+HPXO	(don't know what that last O is.)
	
	looks like the source chk can be any number and the destination chunk must be 1.
	DMAC_CTRLA_SCSIZE_CHK_8
	DMAC_CTRLA_DCSIZE_CHK_1
	will work perfectly.
	
	What must be happening when source/destination CHK is 4 or 8 is the first 4 (or 8) bytes are transferred
	to the transfer holding register and since it happens so fast only the last transferred byte is actually transmitted.
	*/
	
	DMAC->DMAC_CH_NUM[1].DMAC_CTRLB = 0		// control B register				pg. 376
		| DMAC_CTRLB_SRC_DSCR_FETCH_DISABLE		// FETCH Field: all 4 combinations work (src/dst combinations)
		| DMAC_CTRLB_DST_DSCR_FETCH_DISABLE
		//| DMAC_CTRLB_SRC_DSCR_FETCH_FROM_MEM
		//| DMAC_CTRLB_DST_DSCR_FETCH_FROM_MEM
		| DMAC_CTRLB_FC_MEM2PER_DMA_FC
		| DMAC_CTRLB_SRC_INCR_INCREMENTING
		| DMAC_CTRLB_DST_INCR_FIXED
		| DMAC_CTRLB_IEN 					// ???
	;
	DMAC->DMAC_CH_NUM[1].DMAC_CFG = 0		// config register					pg. 378
		| DMAC_CFG_SRC_PER(0)
		| DMAC_CFG_DST_PER(11)				// transmit to USART0
		| DMAC_CFG_SRC_H2SEL_SW				// source uses software handshaking
		| DMAC_CFG_DST_H2SEL_HW				// destination uses hardware handshaking
		| DMAC_CFG_SOD_ENABLE
	;
	/* After Experimenting:
	destination must use hardware handshaking.
	source can be either (doesn't matter)
	DST_PER(n): n must be 11 (USART0)
	SRC_PER(n): n can be anything
	
	DMAC_CTRLB_IEN & DMAC_CFG_SOD_ENABLE: don't make a difference if commented out or not.
	*/
	
	DMAC->DMAC_CHER = DMAC_CHER_ENA1;		// enable ch. 1						pg. 368
	/*
	interesting that this works without doing any single/chunk source/destination transfer requests.
	enabling the channel is enough.
	*/
	
	/*
	while(DMAC->DMAC_CHSR & DMAC_CHSR_ENA1){	// wait for transaction to finish	pg. 370
		// since this works, no point of waiting.
	}
	*/
}









