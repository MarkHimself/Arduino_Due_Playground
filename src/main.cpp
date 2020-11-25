#include <Arduino.h>

#include "pwm_ex.h"

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




uint32_t b[32];
uint16_t pwm_duty_cycles[5] = {10, 800, 50, 200, 122};
uint8_t loc = 0;

void setup() {
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(115200);
	reset_PWM_Controller();
	setup_PIOC6_PWM();
	setup_pwm_ch_2_int();
}

void loop() {
	int delay_time = 100;
	enable_pwm_ch_2_int();
	delay(delay_time);
}


void PWM_Handler(){
	uint32_t pwmIntStatus = PWM->PWM_ISR1;
	if (pwmIntStatus & PWM_ISR1_CHID2){
		if (loc < 5){
			write_PIOC6_PWM_Value(pwm_duty_cycles[loc]);
			loc++;
		}
		else{
			disable_pwm_ch_2_int();
			write_PIOC6_PWM_Value(0);
			loc = 0;
		}
	}
}








