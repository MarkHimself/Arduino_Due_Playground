#include <Arduino.h>
#include <cstdint>

#include "timer_counter.h"
#include "i2c_ex.h"

#define MPU9250_GYRO_CONFIG	0x1B
#define MPU9250_GYRO_CONFIG_GYRO_FS_SEL(a)	(0x18 & (a << 3))


/*
setting up I2C
	- Currently have the most basic i2c functions
	- I2C hangs are successfully detected (at least i think so)
	- I2C hang recovery is implemented.
		- Should I2C be disabled/reset if a hang occurs?
			- i'm going with YES just in case the problem occurs with the SAM3X8E I2C peripheral somehow
			- reset is pretty invisible to the user
				- the previous clock rate is saved between the reset.
		- Testing Procedure
			- hook up sensor and read some data.
			- Briefly connect a GROUND wire to SDA or SLA or both
			- Read data should be all 0 and FAILED.
			- disconnect the GROUND wire
			- I2C should recover.

to do:
	- implement non-blocking i2c functions
	- implement interrupt driven i2c transactions
	- implement DMA driven i2c transactions
*/

void setup() {
	
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
	Serial.begin(115200);
	
	setup_I2C0_master();
	setup_PIOA18_as_TWI0_TWCK0();
	setup_PIOA17_as_TWI0_TWD0();
	//I2C0_Clock_Rate_100khz();
	I2C0_Clock_Rate_khz(400);
	
}

uint8_t readValue = 0;
uint8_t readBuf[6];

int numHanged = 0;
int hangType = 0;

void loop() {
	
	
	
	// 115	0	19	68	0	29	
	
	//if (!I2C0_isBusHanged()) {
		Serial.println("start read");
		read_I2C0_blocking(0x68, 0x75, readBuf, 6);		//  (try to) read some bytes from device.
	//}
	
	for (uint8_t i = 0; i < 6; i++){
		Serial.print(readBuf[i]);
		Serial.print("\t");
		readBuf[i] = 0;						// set the read data to 0 to know if the data was successfully received next time.
	}
	Serial.print("\n");
	//delay_ms(10);
	
	if (!I2C0_transactionSucceeded()){		// if the transaction failed
		Serial.println("failed\n");
		
		if (I2C0_isBusHanged()){			// if the I2C bus is hanged
			
			if (numHanged < 10){			// don't attempt to free the I2C bus for 10 loops
				numHanged++;
			}
			else{
				numHanged = 0;
				hangType = I2C0_checkHangType();	// get the I2C bus hang type
				Serial.print("hang type: ");
				Serial.println(hangType);
				I2C0_FreeBusHang();					// attempt to free the bus.
				if (I2C0_isBusHanged()){
					Serial.println("still hanged");
					Serial.print("hang type again: ");
					Serial.println(I2C0_checkHangType());
				}
				else {
					Serial.println("it got freed!");
					//delay_ms(400);				// short delay if it got freed to read the message...
				}
			}
			
		}
		
	}
	
	
	
	/*
	Serial.print("writing 0x01: ");
	Serial.println(MPU9250_GYRO_CONFIG_GYRO_FS_SEL(0x01));
	write_I2C0_blocking(0x68, MPU9250_GYRO_CONFIG, MPU9250_GYRO_CONFIG_GYRO_FS_SEL(0x01));
	delay_ms(1);
	readValue = read_I2C0_blocking(0x68, MPU9250_GYRO_CONFIG);
	//delay_ms(1);
	Serial.print("read: ");
	Serial.println(readValue);
	//delay_ms(5);
	
	Serial.print("writing 0x02: ");
	Serial.println(MPU9250_GYRO_CONFIG_GYRO_FS_SEL(0x02));
	write_I2C0_blocking(0x68, MPU9250_GYRO_CONFIG, MPU9250_GYRO_CONFIG_GYRO_FS_SEL(0x02));
	delay_ms(1);
	readValue = read_I2C0_blocking(0x68, MPU9250_GYRO_CONFIG);
	//delay_ms(1);
	Serial.print("read: ");
	Serial.println(readValue);
	*/
	
	/*
	write_I2C0_blocking(115, 117);
	delay_ms(2);
	write_I2C0_blocking(0x68, 117);
	delay_ms(2);
	write_I2C0_blocking(0x69, 0xAA);
	delay_ms(100);
	
	write_I2C0_blocking(0x68, 107, 0x06);
	delay_ms(50);
	
	//write_I2C0_blocking(0x68, 117);
	delay_ms(1);
	readValue = read_I2C0_blocking(0x68, 0x75);		// 0x75 = 117
	Serial.println(readValue);
	delay_ms(8);
	*/
	/*
	read_I2C0_blocking(0x68, 0x75, readBuf, 6);
	//delay_ms(1);
	
	for (uint8_t i = 0; i < 6; i++){
		Serial.print(readBuf[i]);
		Serial.print("\t");
	}
	Serial.print("\n");
	*/
}




