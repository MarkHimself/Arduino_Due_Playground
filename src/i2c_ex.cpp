#include "i2c_ex.h"

// Set up I2C0 and pins

uint32_t hang = 0;

void hang_reset(){
	hang = 0;
}

bool hang_check(uint8_t where){
	hang++;
	if (hang > 100000){
		hang = 0;
		Serial.print("hanged\t");
		Serial.println(where);
		return true;
	}
	return false;
}

void setup_I2C0_master(){
	
	PMC->PMC_PCER0 = PMC_PCER0_PID22;	// clock for TWI0						pg. 38, 542
	TWI0->TWI_CR = TWI_CR_SWRST;	// Reset TWI0								pg. 736
	asm volatile("nop");			// wait a clock cycle for software reset to occur. probably not needed but why not.
	
	TWI0->TWI_CR = 0				// control reg.								pg. 736
		| TWI_CR_SVDIS				// disable slave mode
	;
	
	TWI0->TWI_CR = 0				// control reg.								pg. 736
		//| TWI_CR_START				// 
		//| TWI_CR_STOP
		| TWI_CR_MSEN				// master mode enabled.
	;
	
	// MMR gets written in the I2C send/receive functions
	TWI0->TWI_MMR = 0;				// master mode register - reset it			pg. 738
	
	
	TWI0->TWI_CWGR = 0				// clock waveform generator reg.			pg. 741
		| TWI_CWGR_CLDIV(100)		// clock low divider
		| TWI_CWGR_CHDIV(100)		// clock high divider
		| TWI_CWGR_CKDIV(1)			// clock dividor
	;
	
	// DADR
	// CKDIV, CHDIV, CLDIV
	// SVDIS
	// MSEN
	
	
}

// still needs to get done.
bool I2C0_Clock_Rate_khz(uint16_t f){
	
	return false;
}

// TWCK0 is Peripheral A
// Arduino Digital Pin D71 (SCL1)
void setup_PIOA18_as_TWI0_TWCK0(){
	// PIOA is instance i.d. 11													pg. 38
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// enable clock for PIOA				pg. 542
	PIOA->PIO_WPMR = 0x50494F;			// unlock write protect					pg. 674
	
	PIOA->PIO_PUER = PIO_PUER_P18;		// enable pull up						pg. 654
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P25);	// 0 = Peripheral A		pg. 656
	PIOA->PIO_PDR = PIO_PDR_P18;	// disable PIO control of pin. (Peripheral controls it)	pg. 634
}

// TWD0 is Peripheral A
// Arduino Digital Pin D70 (SDA1)
void setup_PIOA17_as_TWI0_TWD0(){
	// PIOA is instance i.d. 11													pg. 38
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// enable clock for PIOA				pg. 542
	PIOA->PIO_WPMR = 0x50494F;			// unlock write protect					pg. 674
	
	PIOA->PIO_PUER = PIO_PUER_P17;		// enable pull up						pg. 655
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P17);	// 0 = Peripheral A		pg. 656
	PIOA->PIO_PDR = PIO_PDR_P17;	// disable PIO control of pin. (Peripheral controls it)	pg. 634
}

// still needs to get done.
bool I2C0_Enabled_Status(){
	
	return false;
}


// *** write through i2c (blocking) *** //

void write_I2C0_blocking(uint8_t devAddress, uint8_t data){
	// instructions on page 719
	uint32_t i2c0_status = 0;
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_NONE
		| (0 << 12)					// master write mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	// the start condition isn't need but won't hurt to use it - it is placed automatically either way.
	//TWI0->TWI_CR = TWI_CR_START;	// send start condition.					pg. 736
	TWI0->TWI_THR = TWI_THR_TXDATA(data);	// transmit holding reg.			pg. 749
	TWI0->TWI_CR = TWI_CR_STOP;				// Send a stop condition			pg. 736
	
	do{
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
	} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
	}
}

void write_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t data){
	// instructions on page 721
	uint32_t i2c0_status = 0;
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_1_BYTE		// have an 8-bit (1 byte) register address to write to
		| (0 << 12)					// master write mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	TWI0->TWI_IADR = TWI_IADR_IADR(regAddress);	// register to write to			pg. 740
	
	// the start condition isn't need but won't hurt to use it - it is placed automatically either way.
	TWI0->TWI_CR = TWI_CR_START;	// send start condition.					pg. 736
	
	TWI0->TWI_THR = TWI_THR_TXDATA(data); // transfer the data.					pg. 749
	
	TWI0->TWI_CR = TWI_CR_STOP;				// Send a stop condition			pg. 736
	
	do{											// wait
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
	} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
	}
	
}

void write_I2C0_blocking(uint8_t devAddress, uint8_t *buffer, uint16_t count){
	// instructions on page 721
	uint32_t i2c0_status = 0;
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_NONE
		| (0 << 12)					// master write mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	// the start condition isn't need but won't hurt to use it - it is placed automatically either way.
	TWI0->TWI_CR = TWI_CR_START;	// send start condition.					pg. 736
	for (uint16_t i = 0; i < count; i++){
		TWI0->TWI_THR = TWI_THR_TXDATA(buffer[i]);	// transfer the data.	pg. 749
		
		do{											// wait
			i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
		} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
		
	}
	
	TWI0->TWI_CR = TWI_CR_STOP;				// Send a stop condition			pg. 736
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
	}
}

void write_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t *buffer, uint16_t count){
	// instructions on page 721
	uint32_t i2c0_status = 0;
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_1_BYTE		// internal register address
		| (0 << 12)					// master write mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	TWI0->TWI_IADR = TWI_IADR_IADR(regAddress);	// register to write to			pg. 740
	
	// the start condition isn't need but won't hurt to use it - it is placed automatically either way.
	TWI0->TWI_CR = TWI_CR_START;	// send start condition.					pg. 736
	for (uint16_t i = 0; i < count; i++){
		TWI0->TWI_THR = TWI_THR_TXDATA(buffer[i]);	// transfer the data.	pg. 749
		
		do{											// wait
			i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
		} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
		
	}
	
	TWI0->TWI_CR = TWI_CR_STOP;				// Send a stop condition			pg. 736
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
	}
}


// *** read through i2c (blocking) *** //

uint8_t read_I2C0_blocking(uint8_t devAddress){
	// instructions on page 722
	// read from the device address. (this function is pretty useless...)
	
	uint32_t i2c0_status = 0;
	uint8_t readVal = 0;
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_NONE		// no internal registers to read from
		| TWI_MMR_MREAD				// master read mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	TWI0->TWI_CR = 0				// send start/stop.							pg. 736
		| TWI_CR_START
		| TWI_CR_STOP
	;
	
	do{											// wait
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
	} while (!(i2c0_status & TWI_SR_RXRDY));	// run loop until RXRDY is ready 
	
	readVal = TWI0->TWI_RHR;					// receive holding register.	pg. 748
	
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
	}
	return readVal;
}

uint8_t read_I2C0_blocking(uint8_t devAddress, uint8_t regAddress){
	// instructions on page 723
	uint32_t i2c0_status = 0;
	uint8_t readVal = 0;
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_1_BYTE		// 1 byte of internal address space.
		| TWI_MMR_MREAD				// master read mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	TWI0->TWI_IADR = regAddress;	// internal address to read from			pg. 740
	
	TWI0->TWI_CR = 0				// send start/stop.							pg. 736
		| TWI_CR_START
		| TWI_CR_STOP
	;
	
	do{											// wait
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
		if (hang_check(1)) return 0;
	} while (!(i2c0_status & TWI_SR_RXRDY));	// run loop until RXRDY is ready 
	hang_reset();
	readVal = TWI0->TWI_RHR;					// receive holding register.	pg. 748
	
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		if (hang_check(1)) return 0;
	}
	hang_reset();
	return readVal;
}

void read_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t *readBuffer, uint16_t count){
	// instructions on page 724
	uint32_t i2c0_status = 0;
	
	if (count == 0) return;
	if (count == 1){
		readBuffer[0] = read_I2C0_blocking(devAddress, regAddress);
		return;
	}
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_1_BYTE		// 1 byte of internal address space.
		| TWI_MMR_MREAD				// master read mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	TWI0->TWI_IADR = regAddress;	// internal address to read from			pg. 740
	
	
	TWI0->TWI_CR = 0				// send start/stop.							pg. 736
		| TWI_CR_START
	;
	
	for (uint16_t i = 0; i < count; i++){
		do{											// wait
			i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
			if (hang_check(1)) return;
			
		} while (!(i2c0_status & TWI_SR_RXRDY));	// run loop until RXRDY is ready 
		hang_reset();
		readBuffer[i] = TWI0->TWI_RHR;				// receive holding register.	pg. 748
		
		if (i == (count - 2)){						// one more data to read
			TWI0->TWI_CR = TWI_CR_STOP;				// stop condition			pg. 736
		}
	}
	
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		if(hang_check(2)) return;
	}
	hang_reset();
	
}









// write/read through i2c (non-blocking)
bool is_I2C0_Transmit_Available(){
	
	return false;
}

bool write_I2C0_nonblocking(uint8_t val, bool isFirstByte, bool isLastByte){
	
	return false;
}

bool is_I2C0_Data_Available(){
	
	return false;
}

int read_I2C0_nonblocking(){
	
	return 0;
}



// i2c0 interrupts
void I2C0_Enable_Int_TXRDY(){
	
}

void I2C0_Disable_Int_TXRDY(){
	
}

void I2C0_Enable_Int_RXRDY(){
	
}

void I2C0_Disable_Int_RXRDY(){
	
}















