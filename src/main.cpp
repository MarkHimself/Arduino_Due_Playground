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
setting up SPI - and different methods of controlling it
	- blocking spi transfers
	- non-blocking spi transfers
	- interrupt controlled spi transfers (current example is very simple and only for reading a single register on another device)
*/

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
	
}

// only used for spi interrupt example			(this can be cleaned up in a struct/class)
uint8_t toTransfer[2] = {117 | 0x80, 0};
uint8_t received = 0;
uint8_t loc = 0;
uint8_t transLength = 2;
uint8_t last_read_value = 0;		// the isr will only return the last read value.
// if you made the transfer array longer, you would only have the last value read.
// a better implementation would be to have an array of read values.
// this will allow multiple bytes to be read and stored. (probably future example)

bool transactionFinished = false;
bool startNewTransaction = true;

void loop() {
	
	uint8_t temp = 0;
	
	/*
	// spi using a blocking transaction.
	set_PIOD10_State(false);			// read the who am i register of MPU9250
	write_SPI0_blocking(117 | 0x80);	// send the address with a read request
	temp = write_SPI0_blocking(0x00);	// read the value.
	set_PIOD10_State(true);
	Serial.println(temp);
	delay_ms(100);
	*/
	
	/*
	// spi using a non-blocking transaction
	set_PIOD10_State(false);
	write_SPI0_nonblocking(117 | 0x80);
	while(!is_SPI0_Transmit_Available());	// useful stuff can be done while waiting for transaction to finish.
	write_SPI0_nonblocking(0x00);
	while(!is_SPI0_Transmit_Available());	// wait for second transaction to finish.
	set_PIOD10_State(true);
	//while (!is_SPI0_Data_Available());		// ensure there is data. obviously there is (since we waited for transaction to finish).
	temp = read_SPI0_nonblocking();
	Serial.println(temp);
	delay_ms(100);
	*/
	
	
	// only used for spi interrupt example
	if (!transactionFinished & startNewTransaction) {
		startNewTransaction = false;
		delay_ms(100);
		SPI0_Enable_Int_TXEMPTY();
		//write_SPI0_nonblocking(0x00);
	}
	
	if (transactionFinished){
		transactionFinished = false;
		SPI0_Disable_Int_RDRF();
		Serial.println(last_read_value);
		delay_ms(100);
		startNewTransaction = true;
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
		if (loc == 0){						// if starting a new transaction
			set_PIOD10_State(false);		// set the chip select LOW
		}
		write_SPI0_nonblocking(toTransfer[loc]);	// write the first byte
		loc++;										// increment the location of the buffer
		if (loc == transLength){					// if at end of buffer
			SPI0_Disable_Int_TXEMPTY();				// disable the empty interrupt (since we aren't sending anything else)
			SPI0_Enable_Int_RDRF();					// enable the receive data register full - so we can read the last value
			//loc = 0;	// it's reset in the read part of the ISR.
		}
	}
	else if ((status & SPI_SR_RDRF) & (SPI0->SPI_IMR & SPI_IMR_RDRF)){	// last value is ready to be read
		transactionFinished = true;					// the transaction is over
		set_PIOD10_State(true);						// set chip select HIGH
		last_read_value = read_SPI0_nonblocking();	// read the last value
		loc = 0;									// reset the buffer pointer to 0
	}
}




