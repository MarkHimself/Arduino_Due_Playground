#include "i2c_ex.h"
#define BUS_HANG_DETECTION

// Set up I2C0 and pins

int i2c0_clock_period_ns = 0;				// keep track of the clock period so that
bool i2c0_clock_period_ns_known = false;	// calculating it all the time isn't needed

bool i2c0_transactionInProgress = false;	// true when a transaction takes place (blocking, non-blocking interrupt/DMA)
volatile bool i2c0_busHanged = false;		// set true if bus hangs (as determined by timer/interrupt)
bool i2c0_transactionSucceeded = true;		// allows user to verify that transaction succeeded.
											// if false, user can check bus_hanged and take action as needed.

int i2c0_hangType = 0;						// determine the hang type.
/*
	0: no bus hang
	1: SCL = HIGH, SDA = HIGH
	2: SCL = HIGH, SDA = LOW
	3: SCL = LOW, SDA = HIGH
	4: SCL = LOW, SDA = LOW
*/

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
	
	// set to 400 KHz
	I2C0_Clock_Rate_400khz();
	setup_TC2_2_for_interrupts();	// used for bus hang detection
}

// still needs to get done.
void I2C0_Clock_Rate_khz(uint16_t f){
	
	// SCL low period: 	( ( CLDIV x (2^CKDIV) ) + 4 ) x T_MCK
	// SCL high period: ( ( CHDIV x (2^CKDIV) ) + 4 ) x T_MCK
	// T_MCK is the period of the master clock. Since the frequency is 84MHz, T_MCK = 1 / ( 84 * 10^(6) ) s = 11.9 ns
	// T_MCK = 11.9 ns 
	
	// ex: CLDIV=100, CHDIV=100, CKDIV=1
	// low period = high period = ( ( 100 x (2^1) ) + 4 ) x 11.9 ns
	// low period = high period = ( ( 200 ) + 4 ) x 11.9 ns
	// low period = high period = 204 x 11.9 ns = 2427.6 ns
	// low period = high period = 2.4276 us
	// pretty much what is seen with a logic analyzer.
	// (i actually see either 2.4, 2.6 or 2.8 us for the low/high clock values (all intermixed every time. interesting))
	// Calculating the clock frequency f:
	// T_c: Combine low & high clock periods: 2 * 2.4276 us = 4.8552 us = 4.8552 * 10^(-6) s
	// f = 1 / (T_c) = 205964.7 Hz = 206 KHz
	
	// calculating the CLDIV, CHDIV, CKDIV from provided frequency:
	// Assumption: low and high clock periods are the same.
	// Remeber: CLDIV & CHDIV are 8-bits long, CKDIV is 3-bits long
	// 1. T_c = 1/f					Calculate the period of clock cycle
	// 2. T_h = T_l = T_c / 2		The low and high periods are half of the clock period
	// 3. Temp1 = T_h / T_MCK - 4
	// 4. temp1 = CLDIV * 2^CKDIV	solve for CLDIV by iterating through all the possible CKDIV value.
	//								the smallest CKDIV values seem to be the most accurate.
	//								choose the smallest CKDIV value while CLDIV fits into an 8-bit number.
	//								ensure that combination is possible... 1khz is impossible to produce with 84MHz clock.
	
	float f_GHz = f / 1000000.0;
	float T_l_ns = (1.0 / f_GHz) / 2.0;
	float temp1 = (T_l_ns / 11.905) - 4.0;
	
	float cldiv_fl = 0;
	int cldiv_int = 0;
	int ckdiv = 0;
	bool found_combo = false;					// found a combo to set clock speed.
	
	for (uint8_t i = 0; i < 8; ++i){			// i = CKDIV
		cldiv_fl = temp1 / (float)(1 << i);		//2^CKDIV = 1 << i;
		
		// rounding the number seems to give better results...
		if ((cldiv_fl - (int) cldiv_fl) > 0.5){	// worst way to round a number...
			cldiv_fl = ((int) cldiv_fl) + 1;
		}
		else{
			cldiv_fl = (int) cldiv_fl;
		}
		
		if (cldiv_fl < 1){
			cldiv_int = 1;
			ckdiv = i;
			found_combo = true;
			break;
		}
		else if (cldiv_fl < 256){
			cldiv_int = cldiv_fl;
			ckdiv = i;
			found_combo = true;
			break;
		}
		
		if (i == 7 && cldiv_fl > 255){		// super low clock speed.
			cldiv_int = 255;
			ckdiv = i;
			found_combo = true;
			break;
		}
	}
	
	if (found_combo){
		TWI0->TWI_CWGR = 0				// clock waveform generator reg.			pg. 741
			| TWI_CWGR_CLDIV(cldiv_int)		// clock low divider
			| TWI_CWGR_CHDIV(cldiv_int)		// clock high divider
			| TWI_CWGR_CKDIV(ckdiv)			// clock dividor
		;
	}
	else{								// if couldn't find a valid combo, set to 400 khz
		I2C0_Clock_Rate_400khz();
	}
	
	i2c0_clock_period_ns_known = false;
}

void I2C0_Clock_Rate_400khz(){
	TWI0->TWI_CWGR = 0				// clock waveform generator reg.			pg. 741
		| TWI_CWGR_CLDIV(101)		// clock low divider
		| TWI_CWGR_CHDIV(101)		// clock high divider
		| TWI_CWGR_CKDIV(0)			// clock dividor
	;
	i2c0_clock_period_ns_known = false;
}

void I2C0_Clock_Rate_100khz(){
	TWI0->TWI_CWGR = 0				// clock waveform generator reg.			pg. 741
		| TWI_CWGR_CLDIV(208)		// clock low divider
		| TWI_CWGR_CHDIV(208)		// clock high divider
		| TWI_CWGR_CKDIV(1)			// clock dividor
	;
	i2c0_clock_period_ns_known = false;
}

void I2C0_Clock_Rate_10khz(){
	TWI0->TWI_CWGR = 0				// clock waveform generator reg.			pg. 741
		| TWI_CWGR_CLDIV(131)		// clock low divider
		| TWI_CWGR_CHDIV(131)		// clock high divider
		| TWI_CWGR_CKDIV(5)			// clock dividor
	;
	i2c0_clock_period_ns_known = false;
}

void I2C0_Clock_Rate_1000khz(){
	TWI0->TWI_CWGR = 0				// clock waveform generator reg.			pg. 741
		| TWI_CWGR_CLDIV(38)		// clock low divider
		| TWI_CWGR_CHDIV(38)		// clock high divider
		| TWI_CWGR_CKDIV(0)			// clock dividor
	;
	i2c0_clock_period_ns_known = false;
}

int I2C0_get_Clock_Rate_Period_ns(){
	
	if (i2c0_clock_period_ns_known) return i2c0_clock_period_ns;
	
	int scale_period = 0;	// period without the microcontroller clock
	int clock_period = 0;	// period of each I2C clock tick
	int cldiv = (TWI0->TWI_CWGR & TWI_CWGR_CLDIV(0xFF)) >> 0;
	int chdiv = (TWI0->TWI_CWGR & TWI_CWGR_CHDIV(0xFF)) >> 8;
	int ckdiv = (TWI0->TWI_CWGR & TWI_CWGR_CKDIV(0x07)) >> 16;
	
	
	scale_period = (cldiv + chdiv) * (1 << ckdiv) + 8;	// combining SCL high/low periods without microcontroller clock.
	clock_period = 11.9 * (float) scale_period;
	
	i2c0_clock_period_ns = clock_period;				// save the clock period
	i2c0_clock_period_ns_known = true;
	
	return clock_period;
}


// TWCK0 is Peripheral A
// Arduino Digital Pin D71 (SCL1)
void setup_PIOA18_as_TWI0_TWCK0(){
	// PIOA is instance i.d. 11													pg. 38
	PMC->PMC_PCER0 = PMC_PCER0_PID11;	// enable clock for PIOA				pg. 542
	PIOA->PIO_WPMR = 0x50494F;			// unlock write protect					pg. 674
	
	PIOA->PIO_MDDR = PIO_MDDR_P18;		// disable open-drain					pg. 651
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
	
	PIOA->PIO_MDDR = PIO_MDDR_P17;		// disable open-drain					pg. 651
	PIOA->PIO_PUER = PIO_PUER_P17;		// enable pull up						pg. 655
	PIOA->PIO_ABSR = PIOA->PIO_ABSR & (~PIO_ABSR_P17);	// 0 = Peripheral A		pg. 656
	PIOA->PIO_PDR = PIO_PDR_P17;	// disable PIO control of pin. (Peripheral controls it)	pg. 634
}

// I2C master status.
bool I2C0_Master_Enabled_Status(){
	return TWI0->TWI_CR & TWI_CR_MSEN;		// is master mode enabled?			pg. 736
}


// *** write through i2c (blocking) *** //

void write_I2C0_blocking(uint8_t devAddress, uint8_t data){
	// instructions on page 719
	uint32_t i2c0_status = 0;
	#ifdef BUS_HANG_DETECTION
		i2c0_transactionInProgress = true;
	#endif
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_NONE
		| (0 << 12)					// master write mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	// the start condition isn't need but won't hurt to use it - it is placed automatically either way.
	//TWI0->TWI_CR = TWI_CR_START;	// send start condition.					pg. 736
	TWI0->TWI_THR = TWI_THR_TXDATA(data);	// transmit holding reg.			pg. 749
	TWI0->TWI_CR = TWI_CR_STOP;				// Send a stop condition			pg. 736
	
	#ifdef BUS_HANG_DETECTION
		int approxTime = I2C0_get_approx_transaction_time_ns(2, 9, 2.5);
		start_hang_detect_timer_ns(approxTime);
	#endif
	
	do{
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return;
		#endif
	} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return;
		#endif
	}
	
	#ifdef BUS_HANG_DETECTION
		TC2_2_Stop_Timer();
		i2c0_transactionInProgress = false;
		i2c0_transactionSucceeded = true;			// probably not needed here.
	#endif
}

void write_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t data){
	// instructions on page 721
	uint32_t i2c0_status = 0;
	#ifdef BUS_HANG_DETECTION
		i2c0_transactionInProgress = true;
	#endif
	
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
	
	#ifdef BUS_HANG_DETECTION
		int approxTime = I2C0_get_approx_transaction_time_ns(3, 9, 2.5);
		start_hang_detect_timer_ns(approxTime);
	#endif
	
	do{											// wait
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return;
		#endif
	} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return;
		#endif
	}
	
	#ifdef BUS_HANG_DETECTION
		TC2_2_Stop_Timer();
		i2c0_transactionInProgress = false;
		i2c0_transactionSucceeded = true;			// probably not needed here.
	#endif
}

void write_I2C0_blocking(uint8_t devAddress, uint8_t *buffer, uint16_t count){
	// instructions on page 721
	uint32_t i2c0_status = 0;
	#ifdef BUS_HANG_DETECTION
		i2c0_transactionInProgress = true;
	#endif
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_NONE
		| (0 << 12)					// master write mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	#ifdef BUS_HANG_DETECTION
		int approxTime = I2C0_get_approx_transaction_time_ns(1 + count, 9, 2.5);
		start_hang_detect_timer_ns(approxTime);
	#endif
	
	// the start condition isn't need but won't hurt to use it - it is placed automatically either way.
	TWI0->TWI_CR = TWI_CR_START;	// send start condition.					pg. 736
	for (uint16_t i = 0; i < count; i++){
		TWI0->TWI_THR = TWI_THR_TXDATA(buffer[i]);	// transfer the data.	pg. 749
		
		do{											// wait
			i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
			#ifdef BUS_HANG_DETECTION
				if (i2c0_busHanged) return;
			#endif
		} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
		
	}
	
	TWI0->TWI_CR = TWI_CR_STOP;				// Send a stop condition			pg. 736
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return;
		#endif
	}
	
	#ifdef BUS_HANG_DETECTION
		TC2_2_Stop_Timer();
		i2c0_transactionInProgress = false;
		i2c0_transactionSucceeded = true;			// probably not needed here.
	#endif
}

void write_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t *buffer, uint16_t count){
	// instructions on page 721
	uint32_t i2c0_status = 0;
	#ifdef BUS_HANG_DETECTION
		i2c0_transactionInProgress = true;
	#endif
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_1_BYTE		// internal register address
		| (0 << 12)					// master write mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	TWI0->TWI_IADR = TWI_IADR_IADR(regAddress);	// register to write to			pg. 740
	
	#ifdef BUS_HANG_DETECTION
		int approxTime = I2C0_get_approx_transaction_time_ns(2 + count, 9, 2.5);
		start_hang_detect_timer_ns(approxTime);
	#endif
	
	// the start condition isn't need but won't hurt to use it - it is placed automatically either way.
	TWI0->TWI_CR = TWI_CR_START;	// send start condition.					pg. 736
	for (uint16_t i = 0; i < count; i++){
		TWI0->TWI_THR = TWI_THR_TXDATA(buffer[i]);	// transfer the data.	pg. 749
		
		do{											// wait
			i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
			#ifdef BUS_HANG_DETECTION
				if (i2c0_busHanged) return;
			#endif
		} while (!(i2c0_status & TWI_SR_TXRDY));	// run loop until TXRDY is ready (when data transferred from THR to internal shifter)
		
	}
	
	TWI0->TWI_CR = TWI_CR_STOP;				// Send a stop condition			pg. 736
	
	// basically, wait until the transmission is finished.
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return;
		#endif
	}
	
	#ifdef BUS_HANG_DETECTION
		TC2_2_Stop_Timer();
		i2c0_transactionInProgress = false;
		i2c0_transactionSucceeded = true;			// probably not needed here.
	#endif
}


// *** read through i2c (blocking) *** //

uint8_t read_I2C0_blocking(uint8_t devAddress){
	// instructions on page 722
	// read from the device address. (this function is pretty useless...)
	
	uint32_t i2c0_status = 0;
	uint8_t readVal = 0;
	#ifdef BUS_HANG_DETECTION
		i2c0_transactionInProgress = true;
	#endif
	
	TWI0->TWI_MMR = 0				// master mode reg.							pg. 738
		| TWI_MMR_IADRSZ_NONE		// no internal registers to read from
		| TWI_MMR_MREAD				// master read mode
		| TWI_MMR_DADR(devAddress)	// device address to communicate with
	;
	
	TWI0->TWI_CR = 0				// send start/stop.							pg. 736
		| TWI_CR_START
		| TWI_CR_STOP
	;
	
	#ifdef BUS_HANG_DETECTION
		int approxTime = I2C0_get_approx_transaction_time_ns(2, 9, 2.5);
		start_hang_detect_timer_ns(approxTime);
	#endif
	
	do{											// wait
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return 0;
		#endif
	} while (!(i2c0_status & TWI_SR_RXRDY));	// run loop until RXRDY is ready 
	
	readVal = TWI0->TWI_RHR;					// receive holding register.	pg. 748
	
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return 0;
		#endif
	}
	
	#ifdef BUS_HANG_DETECTION
		TC2_2_Stop_Timer();
		i2c0_transactionInProgress = false;
		i2c0_transactionSucceeded = true;			// probably not needed here.
	#endif
	return readVal;
}

uint8_t read_I2C0_blocking(uint8_t devAddress, uint8_t regAddress){
	// instructions on page 723
	uint32_t i2c0_status = 0;
	uint8_t readVal = 0;
	#ifdef BUS_HANG_DETECTION
		i2c0_transactionInProgress = true;
	#endif
	
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
	
	#ifdef BUS_HANG_DETECTION
		int approxTime = I2C0_get_approx_transaction_time_ns(4, 9, 2.5);
		start_hang_detect_timer_ns(approxTime);
	#endif
	
	do{											// wait
		i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
		
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return 0;
		#endif
	} while (!(i2c0_status & TWI_SR_RXRDY));	// run loop until RXRDY is ready 
	
	readVal = TWI0->TWI_RHR;					// receive holding register.	pg. 748
	
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return 0;
		#endif
	}
	
	#ifdef BUS_HANG_DETECTION
		TC2_2_Stop_Timer();
		i2c0_transactionInProgress = false;
		i2c0_transactionSucceeded = true;			// probably not needed here.
	#endif
	return readVal;
}

void read_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t *readBuffer, uint16_t count){
	// instructions on page 724
	uint32_t i2c0_status = 0;
	#ifdef BUS_HANG_DETECTION
		i2c0_transactionInProgress = true;
	#endif
	
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
	
	#ifdef BUS_HANG_DETECTION
		int approxTime = I2C0_get_approx_transaction_time_ns(3 + count, 9, 5);
		start_hang_detect_timer_ns(approxTime);
	#endif
	
	for (uint16_t i = 0; i < count; i++){
		do{											// wait
			i2c0_status = TWI0->TWI_SR;				// Read status reg.				pg. 742
			
			#ifdef BUS_HANG_DETECTION
				if (i2c0_busHanged) return;
			#endif
		} while (!(i2c0_status & TWI_SR_RXRDY));	// run loop until RXRDY is ready 
		
		readBuffer[i] = TWI0->TWI_RHR;				// receive holding register.	pg. 748
		
		if (i == (count - 2)){						// one more data to read
			TWI0->TWI_CR = TWI_CR_STOP;				// stop condition			pg. 736
		}
	}
	
	while(!(i2c0_status & TWI_SR_TXCOMP)){		// while transmission is not completed
		i2c0_status = TWI0->TWI_SR;				// read status reg.					pg. 742
		
		#ifdef BUS_HANG_DETECTION
			if (i2c0_busHanged) return;
		#endif
	}
	
	#ifdef BUS_HANG_DETECTION
		TC2_2_Stop_Timer();
		i2c0_transactionInProgress = false;
		i2c0_transactionSucceeded = true;			// probably not needed here.
		i2c0_busHanged = false;						// ??
	#endif
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


// *** I2C0 bus hang detection/recovery *** //

int I2C0_get_approx_transaction_time_ns(int numItems, int bitsPerItem, float scale){
	// numItems: number of "bytes" of data
	// scale: amount to scale the time by to take clock stretching into account
	return scale * (float)(I2C0_get_Clock_Rate_Period_ns() * numItems * bitsPerItem);
}

void start_hang_detect_timer_ns(int expire_ns){
	TC2_2_interrupt_in_x_us(expire_ns / 1000.0);
}

void disable_hang_detect_timer(){
	TC2_2_disable_interrupts();
}

// executed by the ISR.
void hang_detected(){
	i2c0_busHanged = true;
	i2c0_transactionInProgress = false;
	i2c0_transactionSucceeded = false;
}

bool I2C0_transactionSucceeded(){
	return i2c0_transactionSucceeded;
}

bool I2C0_isBusHanged(){
	return i2c0_busHanged;
}

// Determine the hang type
int I2C0_checkHangType(){
	
	bool scl = I2C0_getSclValue();
	bool sda = I2C0_getSdaValue();
	
	if (scl & sda){
		i2c0_hangType = 1;
	}
	else if (scl & !sda){
		i2c0_hangType = 2;
	}
	else if (!scl & sda){
		i2c0_hangType = 3;
	}
	else{
		i2c0_hangType = 4;
	}
	return i2c0_hangType;
}

bool I2C0_FreeBusHang(){
	
	bool busFreed = false;
	uint32_t i2c0_twi_cwgr = TWI0->TWI_CWGR;	// save the i2c clock rate		pg. 741
	
	if (i2c0_hangType == 0) return true;
	
	TWI0->TWI_CR = TWI_CR_SWRST;	// Reset TWI0								pg. 736
	
	setup_PIOA18_TWCKL0_as_OpenDrain_Output();
	setup_PIOA17_TWD0_as_OpenDrain_Output();
	set_PIOA18_TWCKL0_value(true);
	set_PIOA17_TWD0_value(true);
	
	if (i2c0_hangType == 2 || i2c0_hangType == 1){
		for (int i = 0; i < 30; ++i){
			set_PIOA18_TWCKL0_value(false);
			delay_ms(1);
			set_PIOA18_TWCKL0_value(true);
			delay_ms(1);
		}
		if (I2C0_getSclValue() && I2C0_getSdaValue()){
			I2C0_generateStartStop();		// generate stop.
			// check scl/sda values
			if (I2C0_getSclValue() && I2C0_getSdaValue()){
				busFreed = true;
			}
		}
	}
	else if (i2c0_hangType == 3){
		for (int i = 0; i < 10; ++i){
			set_PIOA17_TWD0_value(false);
			delay_ms(1);
			set_PIOA17_TWD0_value(true);
			delay_ms(1);
		}for (int i = 0; i < 10; ++i){
			set_PIOA18_TWCKL0_value(false);
			delay_ms(1);
			set_PIOA18_TWCKL0_value(true);
			delay_ms(1);
		}
		if (I2C0_getSclValue() && I2C0_getSdaValue()){
			I2C0_generateStartStop();		// generate stop.
			// check scl/sda values
			if (I2C0_getSclValue() && I2C0_getSdaValue()){
				busFreed = true;
			}
		}
	}
	else if (i2c0_hangType == 4){	// can't do much in here.
		delay_ms(1);				// let's hope its the microcontroller that froze and this fixes it.
		if (I2C0_getSclValue() && I2C0_getSdaValue()){
			I2C0_generateStartStop();		// generate stop.
			// check scl/sda values
			if (I2C0_getSclValue() && I2C0_getSdaValue()){
				busFreed = true;
			}
		}
	}
	
	setup_I2C0_master();				// setup i2c
	TWI0->TWI_CWGR = i2c0_twi_cwgr;		// set clock to previous value			pg. 736
	i2c0_busHanged = !busFreed;
	
	setup_PIOA18_as_TWI0_TWCK0();
	setup_PIOA17_as_TWI0_TWD0();
	return busFreed;
}

void I2C0_generateStartStop(){
	// generate stop condition on the bus
	set_PIOA17_TWD0_value(false);	// start condition
	delay_ms(1);
	set_PIOA17_TWD0_value(true);	// stop condition
	delay_ms(1);
}


// timer setup for making interrupts to detect the bus hang.
// remember: Arduino due timers notations are kinda weird.
// There timers: TC0, TC1, and TC2. each of them has 3 channels.
// for interrupts, they are labeled from TC0 to TC8.

void setup_TC2_2_for_interrupts(){
	// Instance Name: TC8
	// Instance Id: 35
	
	uint32_t temp = 0;
	
	NVIC_SetPriority(TC8_IRQn, 1);		// set priority of interrupt			pg. 164
	NVIC_DisableIRQ(TC8_IRQn);			// disable the IRQ for setup			pg. 164
	
	PMC->PMC_PCER1 = PMC_PCER1_PID35;		// enable clock for TC8				pg. 39, 563
	TC2->TC_WPMR = TC_WPMR_WPKEY(0x54494D);	// unlock write protect				pg. 908
	
	TC2->TC_CHANNEL[2].TC_CCR = TC_CCR_CLKDIS;	// Disable the clock.			pg. 880
	TC2->TC_CHANNEL[2].TC_CMR = 0			// 									pg. 883
		| TC_CMR_TCCLKS_TIMER_CLOCK1		// clock: MCK / 2
		| TC_CMR_BURST_NONE
		| TC_CMR_CPCSTOP					// stop timer when reaches RC
		| TC_CMR_WAVSEL_UP_RC				// Up mode with trigger on RC (for interrupt)
		| TC_CMR_WAVE						// Waveform mode
	;
	
	TC2->TC_CHANNEL[2].TC_IDR = 0xFF;		// disable all interrupts.			pg. 896
	temp = TC2->TC_CHANNEL[2].TC_SR;		// read/clear status register.
	
	TC2->TC_CHANNEL[2].TC_CCR = TC_CCR_CLKEN;	// Enables the clock.			pg. 880
	
	NVIC_EnableIRQ(TC8_IRQn);			// enable the IRQ						pg. 164
}

void TC2_2_interrupt_in_x_us(uint32_t fire_us){
	TC2->TC_CHANNEL[2].TC_RC = fire_us * 42;	// interrupt in x us.					pg. 891
	uint32_t temp = TC2->TC_CHANNEL[2].TC_SR;	//read/clear status reg.		pg. 892
	TC2->TC_CHANNEL[2].TC_IER = TC_IER_CPCS;	// interrupt on RC Compare		pg. 894
	TC2->TC_CHANNEL[2].TC_CCR = 0				// start the clock.				pg. 880
		| TC_CCR_SWTRG				// software trigger - start timer
		| TC_CCR_CLKEN				// enable the clock.
	;	
}

void TC2_2_enable_interrupts(){
	TC2->TC_CHANNEL[2].TC_IER = TC_IER_CPCS;	// interrupt on RC Compare		pg. 894
}

void TC2_2_disable_interrupts(){
	TC2->TC_CHANNEL[2].TC_IDR = TC_IDR_CPCS;	// disable interrupt on RC Compare	pg. 896
}

void TC2_2_Stop_Timer(){
	TC2->TC_CHANNEL[2].TC_CCR = TC_CCR_CLKDIS;	// disable the clock			pg. 880
	TC2->TC_CHANNEL[2].TC_IDR = TC_IDR_CPCS;	// disable interrupt on RC Compare	pg. 896
}

/*
is it possible for the i2c_done interrupt to execute while inside of here and therefore creating
a critical race condition. a hang will be detected and the transfer complete will be set.
should i disable interrupts while inside of here (and in i2c done interrupt)
and then verify that transfer is not complete. ?
*/
void TC8_Handler(){
	uint32_t TC2_2_IMR = TC2->TC_CHANNEL[2].TC_IMR;	// read mask reg.			pg. 898
	uint32_t TC2_2_SR = TC2->TC_CHANNEL[2].TC_SR;	// read/clear status reg.	pg. 892
	
	if ((TC2_2_IMR | TC2_2_SR) & TC_IMR_CPCS){		// ??
		hang_detected();
	}
}

// PIOA 18
bool I2C0_getSclValue(){
	return PIOA->PIO_PDSR & PIO_PDSR_P18;		// I/O level					pg. 645
}

// PIOA 17
bool I2C0_getSdaValue(){
	return PIOA->PIO_PDSR & PIO_PDSR_P17;		// I/O level					pg. 645
}

void setup_PIOA18_TWCKL0_as_OpenDrain_Output(){
	PIOA->PIO_MDDR = PIO_MDDR_P18;		// enable open drain					pg. 650
	PIOA->PIO_PER = PIO_PER_P18;		// PIO controls pin						pg. 633
	PIOA->PIO_OER = PIO_OER_P18;		// enable pin output					pg. 636
}

void set_PIOA18_TWCKL0_value(bool high){
	if (high) 	PIOA->PIO_SODR = PIO_SODR_P18;		// set high (open-drain)	pg. 642
	else 		PIOA->PIO_CODR = PIO_CODR_P18;		// set low					pg. 643
}

void setup_PIOA17_TWD0_as_OpenDrain_Output(){
	PIOA->PIO_MDDR = PIO_MDDR_P17;		// enable open drain					pg. 650
	PIOA->PIO_PER = PIO_PER_P17;		// PIO controls pin						pg. 633
	PIOA->PIO_OER = PIO_OER_P17;		// enable pin output					pg. 636
}

void set_PIOA17_TWD0_value(bool high){
	if (high) 	PIOA->PIO_SODR = PIO_SODR_P17;		// set high (open-drain)	pg. 642
	else 		PIOA->PIO_CODR = PIO_CODR_P17;		// set low					pg. 643
}






