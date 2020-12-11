#include "spi_ex.h"


/*
SPI0 is instance i.d. 24
Master Mode.
8 bits per transfer (SAM3x8e supports from 8 to 16 bits - in case you want/need to use more)
chip select controlled through software - The SAM3X8E does offer SPI controlled nCS pins but that
requires the use of interrupts/DMA to control SPI transactions.
*/

void setup_SPI0_Master(uint8_t spi_mode){
	uint32_t temp = 0;
	
	PMC->PMC_PCER0 = PMC_PCER0_PID24;	// clock for SPI0						pg. 679, 38, 542
	SPI0->SPI_WPMR = 0x535049;			// unlock write protect					pg. 706
	temp = SPI0->SPI_WPSR;				// clear status reg.					pg. 707
	SPI0->SPI_CR = SPI_CR_SWRST;		// reset SPI0							pg. 693
	
	
	if ((spi_mode == 2) | (spi_mode == 3)) temp = SPI_CSR_CPOL;
	if ((spi_mode == 0) | (spi_mode == 2)) temp |= SPI_CSR_NCPHA;
	
	// i'm going to control the chip select pin through software so this is generic. but important for the spi mode.
	SPI0->SPI_CSR[0] = 0			// chip select register						pg. 703
		| temp						// spi mode
		| SPI_CSR_BITS_8_BIT		// 8 bits
		| SPI_CSR_SCBR(84)			// 84MHz / 84 = 1 MHz baud rate for SPI clock
	;
	
	SPI0->SPI_MR = 0				// mode register							pg. 694
		| SPI_MR_MSTR				// master mode
		| SPI_MR_MODFDIS			// mode fault disable
	;
	
	temp = SPI0->SPI_SR;			// clear status register					pg. 698
	
	SPI0->SPI_CR = SPI_CR_SPIEN;		// enable SPI0
}

// if frequency is 0, fail.
// frequency shall be between 329 KHz and 84000 KHz
bool SPI0_Clock_Rate_khz(uint16_t f){
	
	uint8_t scbr = 0;
	
	if (f == 0) return false;
	
	// verify that no data is being transmitted.
	if (!(SPI0->SPI_SR & SPI_SR_TXEMPTY)){	// if transmitter buffers are not empty		pg. 698
		return false;
	}
	
	scbr = 84000 / f;
	if (scbr == 0) {
		return false;
	}
	
	SPI0->SPI_CSR[0] = SPI0->SPI_CSR[0] & (~SPI_CSR_SCBR(0xFF));	// clear the baud rate	pg. 703
	SPI0->SPI_CSR[0] |= SPI_CSR_SCBR(scbr);	// set the new baud rate			pg. 703
	return true;
}

// MISO is Peripheral A
// Arduino Digital Pin D74
void setup_PIOA25_as_SPI0_MISO(){
	// PIOA is instance i.d. 11													pg. 38
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// enable clock for PIOA				pg. 542
	PIOA->PIO_WPMR = 0x50494F;			// unlock write protect					pg. 674
	
	PIOA->PIO_PUDR = PIO_PUDR_P25;		// disable pull up						pg. 653
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P25);	// 0 = Peripheral A		pg. 656
	PIOA->PIO_PDR = PIO_PDR_P25;	// disable PIO control of pin. (Peripheral controls it)	pg. 634
}

// MOSI is Peripheral A
// Arduino Digital Pin D75
void setup_PIOA26_as_SPI0_MOSI(){
	// PIOA is instance i.d. 11													pg. 38
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// enable clock for PIOA				pg. 542
	PIOA->PIO_WPMR = 0x50494F;			// unlock write protect					pg. 674
	
	PIOA->PIO_PUDR = PIO_PUDR_P26;		// disable pull up						pg. 653
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P26);	// 0 = Peripheral A		pg. 656
	PIOA->PIO_PDR = PIO_PDR_P26;	// disable PIO control of pin. (Peripheral controls it)	pg. 634
}

// SCK is Peripheral A
// Arduino Digital Pin D76
void setup_PIOA27_as_SPI0_SCK(){
	// PIOA is instance i.d. 11													pg. 38
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// enable clock for PIOA				pg. 542
	PIOA->PIO_WPMR = 0x50494F;			// unlock write protect					pg. 674
	
	PIOA->PIO_PUDR = PIO_PUDR_P27;		// disable pull up						pg. 653
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P27);	// 0 = Peripheral A		pg. 656
	PIOA->PIO_PDR = PIO_PDR_P27;	// disable PIO control of pin. (Peripheral controls it)	pg. 634
}

bool SPI0_Enabled_Status(){
	return SPI0->SPI_SR & SPI_SR_SPIENS;	// check if spi0 is enabled.		pg. 698
}


uint8_t write_SPI0_blocking(uint8_t val){
	
	SPI0->SPI_TDR = val;			// transmit data through SPI				pg. 697
	
	// if SPI is not enabled, return 0 (avoid waiting forever for transaction to finish)
	if (!(SPI0->SPI_SR & SPI_SR_SPIENS)){	// check if SPI0 is not enabled.	pg. 698
		return 0;
	}
	
	// wait until the receive register is not empty (wait until it's full)
	while (!(SPI0->SPI_SR & SPI_SR_RDRF)){	// 									pg. 698
		// wait.
	}
	
	return SPI0->SPI_RDR;			// return the read data						pg. 696
}


bool is_SPI0_Transmit_Available(){
	return (SPI0->SPI_SR & SPI_SR_TXEMPTY);	// return true if transmitter is empty	pg. 698
}

// send a byte through SPI without waiting for transaction to finish.
// entire spi transmit buffer must be empty.
bool write_SPI0_nonblocking(uint8_t val){
	if (!(SPI0->SPI_SR & SPI_SR_TXEMPTY)){	// if transmitting registers aren't empty	pg. 698
		return false;						// the transaction failed.
	}
	uint32_t temp = SPI0->SPI_RDR;			// clear the received info (if anything was received before)	pg. 696
	SPI0->SPI_TDR = val;					// transmit the byte				pg. 697
}

bool is_SPI0_Data_Available(){
	return SPI0->SPI_SR & SPI_SR_RDRF;		// return whether there is data available	pg. 698
}

// 0-255 if successful (there is data to read)
// -1 if no data to read.
int read_SPI0_nonblocking(){
	if (SPI0->SPI_SR & SPI_SR_RDRF){		// verify receive register has data	pg. 698
		return SPI0->SPI_RDR;				// return the received data			pg. 696
	}
	return -1;	// no data to return
}


void SPI0_Enable_Int_TXEMPTY(){
	SPI0->SPI_IER = SPI_IER_TXEMPTY;		// unable the txempty interrupt		pg. 700
}

void SPI0_Disable_Int_TXEMPTY(){
	SPI0->SPI_IDR = SPI_IDR_TXEMPTY;		// unable the txempty interrupt		pg. 701
}

// Receive Data Register Full
void SPI0_Enable_Int_RDRF(){
	SPI0->SPI_IER = SPI_IER_RDRF;		// unable the RDRF interrupt		pg. 700
}

// Receive Data Register Full
void SPI0_Disable_Int_RDRF(){
	SPI0->SPI_IDR = SPI_IDR_RDRF;		// unable the RDRF interrupt		pg. 701
}

// *** AHB DMA on channel: Transmit Ch. 4, Receive Ch. 5 *** //

// transmit
// SPI0 transmit is DMA channel interface number 1
void setup_SPI0_Tx_DMA(){
	DMAC->DMAC_EN = DMAC_EN_ENABLE;		// enable the DMA Controller			pg. 360
	DMAC->DMAC_CHDR = DMAC_CHDR_DIS4;	// disable channel 4.					pg. 369
	while (DMAC->DMAC_CHSR & DMAC_CHSR_ENA4){};	// wait for channel 4 to get disabled	pg. 370
}

// receive
// SPI0 receive is DMA channel interface number 2
void setup_SPI0_Rx_DMA(){
	DMAC->DMAC_CHDR = DMAC_CHDR_DIS5;	// disable channel 5.					pg. 369
	while (DMAC->DMAC_CHSR & DMAC_CHSR_ENA5){};	// wait for channel 5 to get disabled	pg. 370
}

// start the transaction.
void start_SPI0_DMA(uint8_t *tx, uint8_t *rx, uint8_t count){
	bool toReceive = !(rx == NULL);	
	
	
	DMAC->DMAC_CH_NUM[4].DMAC_SADDR = (uint32_t)tx;		// source address		pg. 371
	DMAC->DMAC_CH_NUM[4].DMAC_DADDR = (uint32_t)&SPI0->SPI_TDR;	// destination address is SPI0 transmit register	pg. 697, 372
	DMAC->DMAC_CH_NUM[4].DMAC_DSCR = 0;					// no descriptor		pg. 373
	
	DMAC->DMAC_CH_NUM[4].DMAC_CTRLA = 0		// CONTROL A reg					pg. 374
		| DMAC_CTRLA_BTSIZE(count)			// amount to transfer
		| DMAC_CTRLA_DCSIZE_CHK_1			// 1 data transfer each peripheral request
		| DMAC_CTRLA_SRC_WIDTH_BYTE			// source is a byte
		//| DMAC_CTRLA_DST_WIDTH_WORD			// destination is a word
		| DMAC_CTRLA_DST_WIDTH_BYTE			// destination is a BYTE
	;
	
	DMAC->DMAC_CH_NUM[4].DMAC_CTRLB = 0		// CONTROL B reg					pg. 376
		| DMAC_CTRLB_FC_MEM2PER_DMA_FC		// memory to peripheral
		| DMAC_CTRLB_SRC_INCR_INCREMENTING	// increment the source
		| DMAC_CTRLB_DST_INCR_FIXED			// keep destination fixed
	;
	
	DMAC->DMAC_CH_NUM[4].DMAC_CFG = 0		// configuration register			pg. 378
		//| DMAC_CFG_SRC_PER(0)				// source is not used.
		| DMAC_CFG_DST_PER(1)				// transmit to SPI0 (SPI0 is destination)
		| DMAC_CFG_SRC_H2SEL_SW				// source uses software handshaking
		| DMAC_CFG_DST_H2SEL_HW				// destination uses hardware handshaking. 
		// handshake with the destination (the transmit register tells the DMAC when to send the next byte)
		| DMAC_CFG_SOD						// Stop when done
	;
	
	if (toReceive){
		DMAC->DMAC_CH_NUM[5].DMAC_SADDR = (uint32_t)&SPI0->SPI_RDR;		// source address is spi receive register		pg. 696, 371
		DMAC->DMAC_CH_NUM[5].DMAC_DADDR = (uint32_t)rx;					// destination address is user buffer			pg. 372
		DMAC->DMAC_CH_NUM[5].DMAC_DSCR = 0;								// no descriptor		pg. 373
		
		DMAC->DMAC_CH_NUM[5].DMAC_CTRLA = 0		// CONTROL A reg					pg. 374
			| DMAC_CTRLA_BTSIZE(count)			// amount to transfer
			| DMAC_CTRLA_DCSIZE_CHK_1			// 1 data transfer each peripheral request
			//| DMAC_CTRLA_SRC_WIDTH_WORD			// source is a word
			| DMAC_CTRLA_SRC_WIDTH_BYTE			// source is a BYTE
			| DMAC_CTRLA_DST_WIDTH_BYTE			// destination is a byte
		;
		
		DMAC->DMAC_CH_NUM[5].DMAC_CTRLB = 0		// CONTROL B reg					pg. 376
			| DMAC_CTRLB_FC_PER2MEM_DMA_FC		// peripheral to memory
			| DMAC_CTRLB_SRC_INCR_FIXED			// keep the source fixed
			| DMAC_CTRLB_DST_INCR_INCREMENTING	// increment the destination
		;
		
		DMAC->DMAC_CH_NUM[5].DMAC_CFG = 0		// configuration register			pg. 378
			| DMAC_CFG_SRC_PER(2)				// receive from SPI0 is #2
			//| DMAC_CFG_DST_PER(0)				// not used
			| DMAC_CFG_SRC_H2SEL_HW				// source uses hardware handshaking
			// the source hardware handshakes means the source tells DMAC when it's ready to transfer a byte.
			| DMAC_CFG_DST_H2SEL_SW				// destination uses software handshaking
			| DMAC_CFG_SOD						// Stop when done
		;
	}
	
	if (toReceive){
		// start the send and receive
		DMAC->DMAC_CHER = 0						// channel handler enable register	pg. 368
			| DMAC_CHER_ENA4					// start channel 4 (transmit from memory to spi0)
			| DMAC_CHER_ENA5					// start channel 5 (receive from spi0)
		;
		DMAC->DMAC_EBCIER = 0					// Buffer transfer interrupt		pg. 364
			| DMAC_EBCIER_BTC5					// Interrupt when last received byte is received
		;
	}
	else{
		// start only the send
		DMAC->DMAC_CHER = DMAC_CHER_ENA4;		// channel handler enable register	pg. 368
		DMAC->DMAC_EBCIER = 0					// Buffer transfer interrupt		pg. 364
			| DMAC_EBCIER_BTC4					// Interrupt when last byte is transferred
		;
	}
	
	
}


