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

char myString[] = "send me over\n";
uint8_t loc = 0;

void setup() {
	
	
	//Serial.begin(38400);
	//Serial.println("hi");
	setup_UART();
	setup_PIOA9_as_UART_TX();
	setup_PIOA8_as_UART_RX();
	
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	setup_UART_empty_tx_int();
	
}

void loop() {
	
	for (int i = 50; i < 60; i++){
		delay_ms(1000);
		myString[0] = i;
	}
}

void UART_Handler(){
	uint32_t uart_int_status = UART->UART_SR;		// Read status Register		pg. 764
	if (uart_int_status & UART_SR_TXEMPTY){
		transmit_UART(myString[loc]);
		loc = (loc+1) % sizeof(myString);
	}
}




