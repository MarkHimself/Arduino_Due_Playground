#ifndef I2C_EX_H
#define I2C_EX_H
#include <Arduino.h>

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
*/

void setup_I2C0_master();
bool I2C0_Clock_Rate_khz(uint16_t f);
void setup_PIOA18_as_TWI0_TWCK0();
void setup_PIOA17_as_TWI0_TWD0();
bool I2C0_Enabled_Status();

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


*/





#endif