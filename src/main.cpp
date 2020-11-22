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

// AHB DMA memory to memory copy.

char myString[] = "send me over\n ooh look its dma working like it should!\n\n";
char myString2[] = "life is good oh yeah is it still?!!!\n\n";
char myString3[sizeof(myString)];	// will try to copy myString into this string through AHB DMA.
uint8_t loc = 0;

void setup() {
	
	
	//Serial.begin(38400);
	//Serial.println("hi");
	
	setup_UART();
	setup_PIOA9_as_UART_TX();
	setup_PIOA8_as_UART_RX();
	
	reset_Timer_Controller();
	setup_timer_for_ms_delay();
	
	setup_TC2_1_for_interrupts();		// use the timer interrupt to trigger DMA transfer.
	TC2_1_interrupt_period(1000);
	TC2_1_enable_interrupts();
	//setup_UART_empty_tx_int();
	
	setup_UART_TX_String_DMA();
	
	
}

void loop() {
	
	// this will trigger the dma transfer every second.
	/*
	for (int i = 50; i < 60; i++){
		delay_ms(1000);
		myString[0] = i;
		start_UART_TX_String_DMA_Transfer(myString, sizeof(myString));
	}
	*/
}

void UART_Handler(){
	uint32_t uart_int_status = UART->UART_SR;		// Read status Register		pg. 764
	if ((uart_int_status & UART_SR_TXEMPTY) | (uart_int_status & UART_SR_TXRDY)){
		transmit_UART(myString[loc]);
		loc = (loc+1) % sizeof(myString);
	}
}

// this triggers the dma transfer every second based on the timer interrupt.
void TC7_Handler(){
	uint32_t TC2_1_int_status = TC2->TC_CHANNEL[1].TC_SR;		// read status to allow for more interrupts.
	
	if (loc == 0) {
		start_UART_TX_String_DMA_Transfer(myString, sizeof(myString));
	}
	else if (loc == 1){
		start_UART_TX_String_DMA_Transfer(myString2, sizeof(myString2));
	}
	loc = (loc + 1) % 2;
}


