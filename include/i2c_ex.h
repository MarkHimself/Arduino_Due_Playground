#ifndef I2C_EX_H
#define I2C_EX_H

#include <Arduino.h>
#include "timer_counter.h"

/*
i2c in master mode.


*/

/* technical data
TWD: TWO-WIRE SERIAL DATA
TWCK: TWO-WIRE SERIAL CLOCK
connect TWD, TWCK to pull-up resistors (Currently, i'm just using the pull-ups on the SAM3X8E pads, but that isn't ideal)
do not program TWD, TWCK as open drain. (done by hardware in TWI mode)

some good info on i2c: http://ww1.microchip.com/downloads/en/DeviceDoc/90003181A.pdf
also describes how to free a bus hang.

Research SCLWS: on page 744

*/

void setup_I2C0_master();
void I2C0_Clock_Rate_khz(uint16_t f);
void I2C0_Clock_Rate_400khz();
void I2C0_Clock_Rate_100khz();
void I2C0_Clock_Rate_10khz();
void I2C0_Clock_Rate_1000khz();
int I2C0_get_Clock_Rate_Period_ns();
void setup_PIOA18_as_TWI0_TWCK0();
void setup_PIOA17_as_TWI0_TWD0();
bool I2C0_Master_Enabled_Status();

// write through i2c (blocking)
void write_I2C0_blocking(uint8_t devAddress, uint8_t data);
void write_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t data);
void write_I2C0_blocking(uint8_t devAddress, uint8_t *buffer, uint16_t count);
void write_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t *buffer, uint16_t count);

// read through i2c (blocking)
uint8_t read_I2C0_blocking(uint8_t devAddress);
uint8_t read_I2C0_blocking(uint8_t devAddress, uint8_t regAddress);
void read_I2C0_blocking(uint8_t devAddress, uint8_t regAddress, uint8_t *readBuffer, uint16_t count);



// i2c0 interrupts
void I2C0_Enable_Int_TXRDY();
void I2C0_Disable_Int_TXRDY();
void I2C0_Enable_Int_RXRDY();
void I2C0_Disable_Int_RXRDY();

// i2c0 bus hang detection/recovery
int I2C0_get_approx_transaction_time_ns(int numItems, int bitsPerItem, float scale);
void start_hang_detect_timer_ns(int expire_ns);
void disable_hang_detect_timer();
void hang_detected();
bool I2C0_transactionSucceeded();
bool I2C0_isBusHanged();
int I2C0_checkHangType();
bool I2C0_FreeBusHang();
void I2C0_generateStartStop();
// hardware specific functions for i2c0 bus hang detection
void setup_TC2_2_for_interrupts();
void TC2_2_interrupt_in_x_us(uint32_t fire_us);
void TC2_2_enable_interrupts();
void TC2_2_disable_interrupts();
void TC2_2_Stop_Timer();
bool I2C0_getSclValue();
bool I2C0_getSdaValue();
void setup_PIOA18_TWCKL0_as_OpenDrain_Output();
void set_PIOA18_TWCKL0_value(bool high);
void setup_PIOA17_TWD0_as_OpenDrain_Output();
void set_PIOA17_TWD0_value(bool high);

/*

How to create an I2C bus hang:
	- touch the wires (using very high pullups is supposed to "help")
	- reset the microcontroller many times (once it resets in the middle of a transaction it can hang)

I2C bus hang types
	- both lines are high
		- I resetted the microcontroller and it started working
	
	- SDA stuck low (SCL is HIGH)
		- 1. Assert a logic 1 on SDA
		- 2. Master sees a logic 0 on SDA
		- 3. Generate a clock pulse (put SCL LOW, then back HIGH)
		- 4. If SDA is 0, go to step 3. If SDA is HIGH, continue.
		- 5. Make a stop condition
		- 6. Check SDA, repeat steps if necessary.
		
	- both lines are LOW
		- I resetted the microcontroller 2 times and it started working. (or once)


I2C bus hang detection
	Every transaction will have an estimated time (T_es) to be completed
		-Estimated completion time based on:
			- clock speed
			- amount of data to send
	The transaction will be started
	T_es will be calculated
	A timer connected to an interrupt will be started
		If the bus hangs:
			The timer will expire
			The interrupt will execute and set a bus_hanged variable
			If I2C type is blocking:
				The blocking function will see the bus_hanged variable and leave the loop.
				Transaction will be set to "Failed"
				Code will return execution to the user
				Bus hang type will be identified by user calling some function
				User may attempt to free the bus hang
			If I2C type is non-blocking:
				Transaction will be set to "Failed"
				Bus hang type will be identified by user calling some function
				User may attempt to free the bus hang
		If the bus does NOT hang (transaction finished successfully):
			The timer will be turned off.
	



*/





#endif