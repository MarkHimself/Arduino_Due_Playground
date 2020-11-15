#include <Arduino.h>
// Arduino Due
// Do random stuff on it to learn it better.
/*
- Input
	- Digital
	- Analog
- Output
	- Digital
- PWM
- Timers
- Sending UART messages to COM
- Receiving UART messages to COM
	- polling
	- interrupts
- Interrupts on PWM
- Interrupts on input pin
- Interrupts on analog value is read
- DMA
	- sending a string through UART to COM port
	- Updating PWM
*/

/* Set up PWM on digital pin 38
it is pin PIOC6
PWM: peripheral B, PWML2 - PWM Channel 2 Output Low
write 0 to 1000 for PWM control.
	0 = Low
	1000 = High
	CPOL looks like it's used backwards.
	Maybe because i'm using PWML and not PWMH
*/


void reset_PWM_Controller();
void setup_PIOC6_PWM();
void write_PIOC6_PWM_Value(uint16_t val);


uint32_t b[32];


void setup() {
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(9600);
	reset_PWM_Controller();
	setup_PIOC6_PWM();
	
}

void loop() {
	int delay_time = 50;
	
	write_PIOC6_PWM_Value(millis() % 1000);
	
	/*
	write_PIOC6_PWM_Value(45);
	delay(delay_time);
	write_PIOC6_PWM_Value(950);
	delay(delay_time);
	write_PIOC6_PWM_Value(2);
	delay(delay_time);
	write_PIOC6_PWM_Value(995);
	delay(delay_time);
	write_PIOC6_PWM_Value(0);
	delay(delay_time);
	write_PIOC6_PWM_Value(500);	
	delay(delay_time);
	write_PIOC6_PWM_Value(1000);
	delay(delay_time);
	*/
}

void reset_PWM_Controller(){
	// reset PWM
	// how?
	
	// clear write protect register
	PIOC->PIO_WPMR = (0x50494F << 8);	// 							pg. 674
	PWM->PWM_WPCR = 0x50574D << 8;		// pwm						pg. 1037
	
	// pioc instance 13
	PMC->PMC_PCER0 = b[13];		// turn on clock for PIOC			pg. 542
	
	// pwm instance 36
	PMC->PMC_PCER1 = b[4];		// turn on clock for PWM			pg. 563
	
}

void setup_PIOC6_PWM(){
	
	// disable pwm on channel 2
	PWM->PWM_DIS = b[2];	// 										pg. 1008
	
	// *** Setup PIOC Pin to use PWM ***
	// disable pull up resistor. through PIO_PUDR					pg. 622
	PIOC->PIO_PUDR = b[6];	// 										pg. 653
	// disable PIO control of PIOC
	PIOC->PIO_PDR = b[6];	// 										pg. 634
	// select peripheral B
	PIOC->PIO_ABSR |= b[6];
	
	// *** setup pwm registers ***
	
	// Not using CLKA, CLKB
	PWM->PWM_CLK = 0;	// 											pg. 1006
	
	PWM->PWM_CH_NUM[2].PWM_CMR = 0 		// channel mode				pg. 1044
		| 0b0011			// use: master_clock / 8
		//| PWM_CMR_CPOL		// Start LOW at beginning of pulse
		// i want it to start high, so not using CPOL.
	;
	
	PWM->PWM_CH_NUM[2].PWM_CDTY = 0;	// duty cycle				pg. 1046
	
	// pwm comparisons.
	PWM->PWM_CMP[2].PWM_CMPM = 0;
	
	
	// pwm channel period will be 1000
	PWM->PWM_CH_NUM[2].PWM_CPRD = 1000;		//						pg. 1048
	
	// enable PWM on channel 2
	PWM->PWM_ENA = b[2];	// 										pg. 1007
}

void write_PIOC6_PWM_Value(uint16_t val){
	if (val > 1000) val = 1000;
	PWM->PWM_CH_NUM[2].PWM_CDTYUPD = val;
}










