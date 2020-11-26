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


// use PDC DMA (Peripheral DMA Controller) to upload the pwm duty cycles.

/*
if i make this a uint32_t buffer with 6 values, the 6 values that get transmitted
are the first 3. first the LS16bits of the 32 bit number and then the MS16bits of
the 32 bit number.
If using a uint32_t buffer with 6 values, plug in 12 into the TCR register.
But: this will result in an extra 0 duty cycle written out between the writes.
This is because first the LS16bits get sent (the duty cycle) and then the MS16bits (0) gets sent for each period.

using sizeof(pwm_duty_cycles) on the uint32_t buffer with 6 values results in 24.
24 makes sense: 6 values each containing 4 bytes: 6x4=24
Putting 24 into the TCR register results in a buffer overflow read and unwanted duty cycles are sent out.

Using a uint16_t buffer behaves exactly as expected. buffer has 6 values. plug in 6 into register.

Answer as i now understand it:
The PWM is a 32 bit register but it holds a 16 bit value. therefore, 16 bit values are used for every 1 buffer size in TCR.
Anyways, use a uint16_t buffer and put the buffer length into the TCR register..
*/
uint16_t pwm_duty_cycles[6] = {20, 700, 50, 200, 122, 0};	// 5 random pwm pulses.
uint16_t oneshot[2] = {441, 0};		// 42 us
uint8_t loc = 0;	// location in buffer if the ISR is used to update duty cycles.

void setup() {
	
	Serial.begin(115200);
	reset_PWM_Controller();
	setup_PIOC6_PWM();
	//setup_pwm_ch_2_int();
	
	setup_pwmSyncPDC_ch_0_PIOC3();
	setup_PDC_for_pwm_ch_0();
	
}

void loop() {
	int delay_time = 5;
	start_pwmSyncPDC_ch_0_PIOC3(pwm_duty_cycles, 6);
	//start_pwmSyncPDC_ch_0_PIOC3(oneshot, 2);
	
	//enable_pwm_ch_2_int();
	delay(delay_time);
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








