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

// Use the AHB DMA to transfer data to the pwm duty cycle register.
/*
i don't know why this code (AHB DMA to syncPWM) does not work.
*/

uint16_t pwm_duty_cycles[6] = {20, 700, 50, 200, 122, 0};	// 5 random pwm pulses.
uint16_t oneshot[2] = {441, 0};		// 42 us
uint8_t loc = 0;	// location in buffer if the ISR is used to update duty cycles.

void setup() {
	
	bool ahb_setup_status = false;
	
	Serial.begin(115200);
	reset_PWM_Controller();
	setup_PIOC6_PWM();
	//setup_pwm_ch_2_int();
	
	setup_pwmSyncPDC_ch_0_PIOC3();	// setting up pwm channel 0 to be sync. (also for DMAC apparently)
	
	//setup_PDC_for_pwm_ch_0();
	
	
	ahb_setup_status = setup_AHB_DMA_for_pwm();
	while (!ahb_setup_status){
		Serial.println("failed\n");
		delay(100);
	}
	
	
}

void loop() {
	int delay_time = 100;
	uint32_t temp = 0;
	//start_pwmSyncPDC_ch_0_PIOC3(pwm_duty_cycles, 6);
	//start_pwmSyncPDC_ch_0_PIOC3(oneshot, 2);
	
	start_AHB_DMA_for_pwm(pwm_duty_cycles, 6);
	temp = PWM->PWM_SCUP;
	for (int i = 0; i < 200; i++){
		//Serial.println(DMAC->DMAC_CHSR);	// 0x00 3E 00 01, not empty and enabled.
		//Serial.println(temp >> 4);
		//delayMicroseconds(1);
		temp = PWM->PWM_SCUP;
	}
	
	//enable_pwm_ch_2_int();
	delay(delay_time);
	Serial.println("in loop\n");
}

// not using pwm interrupts for the PDC DMA on PWM.
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








