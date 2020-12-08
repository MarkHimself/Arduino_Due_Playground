#include <Arduino.h>
#include <cstdint>

#include "timer_counter.h"
#include "uart.h"
#include "ahb_dma_mem.h"
#include "spi_ex.h"

void enable_PIOD();
void set_PIOD10_Output();
void set_PIOD10_State(bool ishigh);

/*
setting up SPI - interrupt transfer
	- provide a buffer of data to transfer
	- provide a buffer that records all received data
	- setting the transaction length to zero and requesting a transaction
		- this dangerous scenario hasn't been tested but is expected to work. safety logic added into ISR.
	- future immprovements:
		- change the ISR to use a pointer instead.
			- multiple SPI_INT_DATA variables can be declared and their address can be assigned to the one to be transferred.
			- for multiple SPI_INT_DATA variables, the transactionInProgress variable may need to be static.?
*/

SPI_INT_DATA<25> spi_int_data_ex;		// create a int data structure capable of transferring up to 5 bytes through spi.

void setup() {
	
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
	Serial.begin(38400);
	
	enable_PIOD();				// set up chip select pin
	set_PIOD10_Output();
	set_PIOD10_State(true);
	
	setup_SPI0_Master(3);		// set up spi in mode 3
	SPI0_Disable_Int_RDRF();
	SPI0_Disable_Int_TXEMPTY();
	setup_PIOA25_as_SPI0_MISO();
	setup_PIOA26_as_SPI0_MOSI();
	setup_PIOA27_as_SPI0_SCK();
	SPI0_Clock_Rate_khz(1000);	// use 1000KHz (1MHz)
	
	// only used for spi interrupt example
	NVIC_EnableIRQ(SPI0_IRQn);		// enable spi0 interrupts					pg. 164
	NVIC_SetPriority(SPI0_IRQn, 0);		// set a pretty important priority but not most important.
	
	// prepare the spi transaction that should take place through interrupts.
	//spi_int_data_ex.toTransfer[0] = 117 | 0x80;	// read the who am i register of MPU9250
	spi_int_data_ex.toTransfer[0] = 85 | 0x80;
	for (int i = 1; i < 25; i++) spi_int_data_ex.toTransfer[i] = 0;
	spi_int_data_ex.setTransLength(10);
	
}

bool do_once = true;

void loop() {
	
	// triggering the transaction software.
	if (!spi_int_data_ex.isTransactionInProgress() /* && some user personal code */ & do_once){
		spi_int_data_ex.setNewTransactionRequest();
		do_once = false;
	}
	
	// triggering the hardware to perform the transaction.
	if (!spi_int_data_ex.isTransactionInProgress() & spi_int_data_ex.isStartNewTransactionRequest()){	// if a transaction has been requested
		spi_int_data_ex.resetNewTransactionRequest();	// reset the request since we are servicing it.
		SPI0_Enable_Int_TXEMPTY();
	}
	
	// transaction over.
	if (spi_int_data_ex.isTransactionFinished()){
		spi_int_data_ex.resetTransactionInProgress();
		spi_int_data_ex.resetTransactionFinished();
		SPI0_Disable_Int_RDRF();
		
		// rest of code is user dependent.
		// print the values.
		for (int i = 0; i < spi_int_data_ex.getTransLength(); i++) {
			Serial.print(spi_int_data_ex.received[i]);
			if (i != (spi_int_data_ex.getTransLength() - 1)) Serial.print("\t");	// why wait like an extra 260us??
		}
		Serial.print("\n");
		
		delay_ms(100);
		spi_int_data_ex.setNewTransactionRequest();		// perform another request - this can be requested here or in another part of the code.
		// (if you don't need to start a new transaction immediately when the previous one finished).
		
		// scan the mpu9250 10 bytes at a time while incrementing the starting location by 1 each time.
		// yeah for fun. would look better if i actually powered and set up the gyro/accel/mag in here also.
		if ((spi_int_data_ex.toTransfer[0] & 127) < 110){
			spi_int_data_ex.toTransfer[0] = ((spi_int_data_ex.toTransfer[0] & 0x7F) + 1) | 0x80;
		}
		else{
			spi_int_data_ex.toTransfer[0] = 0x80;
		}
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
		spi_int_data_ex.setTransactionFinished();							// the transaction is over
		set_PIOD10_State(true);												// set chip select HIGH
		spi_int_data_ex.received[spi_int_data_ex.getLocation() - 1] = read_SPI0_nonblocking();		// read the last value
		spi_int_data_ex.resetLocation();									// reset the buffer pointer to 0
	}
}



