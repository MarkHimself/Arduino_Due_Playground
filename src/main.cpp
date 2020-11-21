#include <Arduino.h>
#include <cstdint>

#include "timer_counter.h"


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


void setup() {
	
	for (uint32_t i = 0; i < 32; i++) b[i] = (1u << i);
	Serial.begin(38400);
	Serial.println("hi");
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
	setup_timer_for_oneshot();
	setup_PIOC25_as_TIOA6();
	
	setup_TC2_1_for_interrupts();
	TC2_1_interrupt_period(1000);
	TC2_1_enable_interrupts();
	
	Serial.println("clock status");
	Serial.println(TC2->TC_CHANNEL[1].TC_SR);
	
}

void loop() {
	/*
	oneshot_TIOA6_us(35);
	delay_ms(1);
	
	oneshot_TIOA6_us(200);
	delay_ms(1);
	
	oneshot_TIOA6_us(1);
	delay_ms(1);
	
	oneshot_TIOA6_42MHz_Clock_Ticks(1);
	//delay_ms(1);
	
	oneshot_TIOA6_us(4);
	delay_ms(1);
	*/
	
	if (get_TC2_1_int_flag()){
		Serial.println("interrupt received");
	}
}





