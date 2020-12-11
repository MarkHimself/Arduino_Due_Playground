#include <Arduino.h>
#include <cstdint>

#include "timer_counter.h"
#include "dma_controller.h"
#include "spi_ex.h"


void enable_PIOD();
void set_PIOD10_Output();
void set_PIOD10_State(bool ishigh);

/*
setting up SPI - AHB DMA transfer
	- using the same class from the SPI Interrupt example
		- better to make a new data structure. even a simple struct would do
	- DMA write only operation:
		- give the DMA the buffer address and the length of the buffer.
		- give the DMA a NULL for the receiving buffer address
		- an interrupt notifies when the write is finished.
	- DMA write/read operation:
		- give the DMA the buffer address and the length of the buffer.
		- give the DMA a buffer for storing received data
		- an interrupt notifies when the write is finished.
*/

// for SPI DMA, something simpler is needed... didn't feel like changing it though... so still using the interrupt data structure.
SPI_INT_DATA<7> spi_int_data_ex;		// create an spi interrupt data structure capable of transferring up to 7 bytes through spi.

void setup() {
	
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
	Serial.begin(38400);
	
	enable_PIOD();				// set up chip select pin
	set_PIOD10_Output();
	set_PIOD10_State(true);
	
	// set up SPI0
	setup_SPI0_Master(3);		// set up spi in mode 3
	SPI0_Disable_Int_RDRF();
	SPI0_Disable_Int_TXEMPTY();
	setup_PIOA25_as_SPI0_MISO();
	setup_PIOA26_as_SPI0_MOSI();
	setup_PIOA27_as_SPI0_SCK();
	SPI0_Clock_Rate_khz(1000);	// use 1000KHz (1MHz)
	
	delay_ms(4);
	
	// only used for spi interrupt example (it is possible to transmit through DMA and receive through SPI interrupts)
	NVIC_EnableIRQ(SPI0_IRQn);		// enable spi0 interrupts					pg. 164
	NVIC_SetPriority(SPI0_IRQn, 0);		// set the most important priority
	
	NVIC_EnableIRQ(DMAC_IRQn);		// enable DMAC interrupts.
	NVIC_SetPriority(DMAC_IRQn, 2);
	
	DMA_power_on();				// power on the DMA.
	DMA_AHB_Unlock_Write_Protect();
	
	// prepare the spi transaction that should take place through interrupts.
	spi_int_data_ex.toTransfer[0] = 117 | 0x80;	// read the who am i register of MPU9250
	for (int i = 1; i < 7; i++) spi_int_data_ex.toTransfer[i] = 0;
	spi_int_data_ex.setTransLength(7);
	// 0	115	0	21	34	0	18		// <---- Output i got using the interrupt code. DMA should (and is) the same.
	
	setup_SPI0_Tx_DMA();
	setup_SPI0_Rx_DMA();
	
	/*
	// using dma to transfer transmit buffer and ISR to read the received values..
	SPI0_Enable_Int_RDRF();	
	set_PIOD10_State(false);									// set the chip select LOW
	start_SPI0_DMA(spi_int_data_ex.toTransfer, NULL, spi_int_data_ex.getTransLength());
	*/
	
	// using dma to transfer transmit buffer and dma to read the received values
	set_PIOD10_State(false);									// set the chip select LOW
	start_SPI0_DMA(spi_int_data_ex.toTransfer, spi_int_data_ex.received, spi_int_data_ex.getTransLength());
	
}

void loop() {
	
	delay_ms(5);
	
	if (spi_int_data_ex.isTransactionFinished()){
		
		for (int i = 0; i < spi_int_data_ex.getTransLength(); i++) {
			Serial.print(spi_int_data_ex.received[i]);
			if (i != (spi_int_data_ex.getTransLength() - 1)) Serial.print("\t");	// why wait like an extra 260us??
		}
		Serial.print("\n");
		delay_ms(10);
		spi_int_data_ex.resetTransactionFinished();
		set_PIOD10_State(false);									// set the chip select LOW to select the MPU9250
		start_SPI0_DMA(spi_int_data_ex.toTransfer, spi_int_data_ex.received, spi_int_data_ex.getTransLength());
	}
}

void enable_PIOD(){
	uint32_t temp = 0;
	// piod is instance id 14													pg. 38
	PMC->PMC_PCER0 = PMC_PCER0_PID14;	// enable clock for PIOD				pg. 542
	PIOD->PIO_WPMR = 0x50494F;			// unlock write protect					pg. 674
	temp = PIOD->PIO_WPSR;				// clear status reg.					pg. 675
}

void set_PIOD10_Output(){
	PIOD->PIO_IDR = PIO_IDR_P10;		// disable interrupt PIOD10 change		pg. 647
	PIOD->PIO_MDDR = PIO_MDDR_P10;		// disable multi drive					pg. 651
	PIOD->PIO_PUDR = PIO_PUDR_P10;		// disable pull up resistor				pg. 653
	PIOD->PIO_PER = PIO_PER_P10;		// pio controls PIOD10					pg. 633
	PIOD->PIO_OER = PIO_OER_P10;		// PIOD10 is an output					pg. 636
	PIOD->PIO_SODR = PIO_SODR_P10;		// set PIOD10 HIGH						pg. 642
	
}

void set_PIOD10_State(bool isHigh){
	if (isHigh) PIOD->PIO_SODR = PIO_SODR_P10;	// set pin high					pg. 642
	else PIOD->PIO_CODR = PIO_CODR_P10;			// set pin low					pg. 643
	//delay_ms(1);
}

// only used for spi interrupt example
void SPI0_Handler(){
	uint32_t status = SPI0->SPI_SR;		// read status reg.						pg. 698
	
	// read the interrupt mask register and make sure the interrupt is actually enabled.
	// SPI0->SPI_IMR tells what caused the interrupt.
	// reading the status register is pointless actually.
	// reading only the status register will not work!
	if ((status & SPI_SR_TXEMPTY) & (SPI0->SPI_IMR & SPI_IMR_TXEMPTY)){	// 		pg. 698, 702
		if (spi_int_data_ex.getLocation() == 0){						// if starting a new transaction
			set_PIOD10_State(false);									// set the chip select LOW
		}
		if (spi_int_data_ex.getLocation() != 0){
			spi_int_data_ex.received[spi_int_data_ex.getLocation() - 1] = read_SPI0_nonblocking();	// read the value that was received when the previous byte was sent.
		}
		
		// probably better to put this in the beginning and nest everything inside it.
		if (spi_int_data_ex.getTransLength() > 0){								// ensure transaction has bytes to transfer.
			write_SPI0_nonblocking(spi_int_data_ex.toTransfer[spi_int_data_ex.getLocation()]);	// write the first byte
		}
		else{
			spi_int_data_ex.setTransactionFinished();							// the transaction is over
			set_PIOD10_State(true);												// set chip select HIGH
			spi_int_data_ex.resetLocation();									// reset the buffer pointer to 0
			SPI0_Disable_Int_TXEMPTY();											// disable the empty interrupt (since we aren't sending anything else)
			return;
		}
		
		spi_int_data_ex.incrementLocation();				// increment the location of the buffer
		if (spi_int_data_ex.isEndOfTransfer()){				// if at end of buffer
			SPI0_Disable_Int_TXEMPTY();						// disable the empty interrupt (since we aren't sending anything else)
			SPI0_Enable_Int_RDRF();							// enable the receive data register full - so we can read the last value
		}
	}
	else if ((status & SPI_SR_RDRF) & (SPI0->SPI_IMR & SPI_IMR_RDRF)){	// last value is ready to be read
		
		// send through dma and read on interrupts
		if (spi_int_data_ex.getLocation() == spi_int_data_ex.getTransLength()){
			spi_int_data_ex.setTransactionFinished();							// the transaction is over
			set_PIOD10_State(true);												// set chip select HIGH
			
		}
		spi_int_data_ex.received[spi_int_data_ex.getLocation()] = read_SPI0_nonblocking();		// read the last value
		spi_int_data_ex.incrementLocation();				// increment the location of the buffer
		
		
		// code that was used solely for the spi interrupt example.
		/*
		spi_int_data_ex.setTransactionFinished();							// the transaction is over
		set_PIOD10_State(true);												// set chip select HIGH
		spi_int_data_ex.received[spi_int_data_ex.getLocation() - 1] = read_SPI0_nonblocking();		// read the last value
		spi_int_data_ex.resetLocation();									// reset the buffer pointer to 0
		*/
	}
}

void DMAC_Handler(){
	uint32_t dma_status = DMAC->DMAC_CHSR;			// channel					pg. 370
	uint32_t dma_ebcimr_mask = DMAC->DMAC_EBCIMR;	// mask reg.				pg. 366
	uint32_t dma_ebcisr_status = DMAC->DMAC_EBCISR;	// status reg.				pg. 367
	
	if (dma_ebcisr_status & (DMAC_EBCISR_BTC4 | DMAC_EBCISR_BTC5)){				// pg. 367
		spi_int_data_ex.setTransactionFinished();
		DMAC->DMAC_EBCIDR = 0	// Disable int reg.								pg. 365
			| DMAC_EBCIDR_BTC4
			| DMAC_EBCIDR_BTC5
		;
		set_PIOD10_State(true);		// set chip select HIGH - ends communication with MPU9250
	}
	
}




