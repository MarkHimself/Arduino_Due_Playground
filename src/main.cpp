#include <Arduino.h>
#include <cstdint>

#include "timer_counter.h"
#include "uart.h"

#define b(r) (1u << (r))

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

void setup() {
	
	
	//Serial.begin(38400);
	//Serial.println("hi");
	setup_UART();
	setup_PIOA9_as_UART_TX();
	setup_PIOA8_as_UART_RX();
	
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
}

void loop() {
	// transmitting numbers and letters through uart to com port.
	for (uint8_t i = 48; i < 127; i++){
		transmit_UART(i);
		delay_ms(10);
		transmit_UART(10);		// transmit new line.
		delay_ms(100);
	}
	delay(3000);
}





