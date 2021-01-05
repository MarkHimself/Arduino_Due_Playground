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
	- I2C hang recovery is not yet implemented.

to do:
	- implement the most robust possible i2c bus hang detection system.
		- and also a way to remove the hang.
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
	
	delay_ms(4000);
}

uint8_t readValue = 0;
uint8_t readBuf[6];

bool disInt = true;

void loop() {
	
	Serial.println("start");
	
	// 115	0	19	68	0	29	
	read_I2C0_blocking(0x68, 0x75, readBuf, 6);
	
	for (uint8_t i = 0; i < 6; i++){
		Serial.print(readBuf[i]);
		Serial.print("\t");
		readBuf[i] = 0;
	}
	Serial.print("\n");
	delay_ms(10);
	
	if (!I2C0_transactionSucceeded()){
		Serial.println("failed\n");
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




