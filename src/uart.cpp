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



