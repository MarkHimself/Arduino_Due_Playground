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







